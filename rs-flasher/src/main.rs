mod segments;

use chrono::DateTime;
use dialoguer::theme::ColorfulTheme;
use dialoguer::{Confirm, Select};
use espflash;
use espflash::connection::reset::{ResetAfterOperation, ResetBeforeOperation};
use espflash::error::Error;
use espflash::flasher::{Flasher, ProgressCallbacks};
use espflash::targets::Chip;
use indicatif::{ProgressBar, ProgressStyle};
use log::{debug, error, info};
use miette::{IntoDiagnostic, Result};
use serde::Deserialize;
use serialport::{available_ports, FlowControl, SerialPortInfo, SerialPortType, UsbPortInfo};
use std::io;
use std::u32;
use espflash::elf::RomSegment;

#[allow(unused_imports)]
use std::io::Write;
use espflash::connection::Port;
use crate::segments::{Device, get_chip, get_segments};

#[cfg(debug_assertions)]
fn setup_logger() {
    let env = env_logger::Env::default().filter_or("KNOMI_LOG_LEVEL", "info");
    env_logger::Builder::from_env(env).init();
}

#[cfg(not(debug_assertions))]
fn setup_logger() {
    let env = env_logger::Env::default().filter_or("KNOMI_LOG_LEVEL", "info");
    env_logger::Builder::from_env(env)
        .format(|buf, record| {
            let style = buf.default_level_style(record.level());
            let timestamp = buf.timestamp();
            let level = record.level();
            writeln!(
                buf,
                "[{timestamp} {level}]: {style}{}{style:#}",
                record.args()
            )
        })
        .init();
}

fn main() -> Result<()> {
    miette::set_panic_hook();

    setup_logger();

    let build_info_str = include_str!("../resources/buildinfo.json");

    let build_info: BuildInfo = serde_json::from_str(build_info_str).unwrap();

    // Display the disclaimer
    let disclaimer = format!(
        r#"
KNOMI Firmware Installer by DiverOfDark
==================================================================

Firmware info: https://github.com/DiverOfDark/KNOMI

Branch: {} ({})
Source Timestamp: {}
Binary Timestamp: {}

==================================================================

This tool will flash alternative KNOMI firmware to your device.
Please put your KNOMI into flash mode (press button and connect cable).

"#,
        build_info.branch,
        build_info.commit,
        format_time(build_info.commit_time),
        format_time(build_info.build_time)
    );

    println!("{}", disclaimer);

    let mut user_input = String::new();

    if confirm_flash().is_ok() {
        let device = select_device();
        if device.is_ok() {
            let result = flash_firmware(device.unwrap());

            if result.is_ok() {
                info!("Firmware flashing completed. Please unplug your KNOMI and plug it back.");
                info!("Press Enter to exit");
                io::stdin().read_line(&mut user_input).unwrap();
            } else {
                let string = result.unwrap_err().to_string();
                error!("Firmware flashing error!");
                error!("{}", string);
                error!("Press Enter to exit");
                io::stdin().read_line(&mut user_input).unwrap();
            }
        }
    } else {
        error!("Firmware flashing canceled.");
        std::process::exit(1); // Exit the program with an error code
    }

    // full flash = espflash.exe write-bin -b 460800 0x1000 firmware_full.bin
    Ok(())
}

fn flash_firmware(device: Device) -> Result<(), Error> {
    let ports = detect_usb_serial_ports(true).unwrap_or_default();
    let port_info = select_serial_port(ports)?;

    // Attempt to open the serial port and set its initial baud rate.
    info!("Serial port: '{}'", port_info.port_name);

    let serial_port = serialport::new(&port_info.port_name, 115_200)
        .flow_control(FlowControl::None)
        .open_native()
        .map_err(Error::from)?;

    // NOTE: since `get_serial_port_info` filters out all PCI Port and Bluetooth
    //       serial ports, we can just pretend these types don't exist here.
    let port_info = match port_info.port_type {
        SerialPortType::UsbPort(info) => info,
        SerialPortType::PciPort | SerialPortType::Unknown => {
            debug!("Matched `SerialPortType::PciPort or ::Unknown`");
            UsbPortInfo {
                vid: 0,
                pid: 0,
                serial_number: None,
                manufacturer: None,
                product: None,
            }
        }
        _ => unreachable!(),
    };

    let chip = get_chip(&device);

    info!("Using chip {}", chip);

    let segments = get_segments(&device);

    for segment in &segments {
        info!("Going to flash at 0x{:06X}: 0x{:06X} bytes", segment.addr, segment.data.len())
    }

    write_firmware(segments, serial_port, port_info, chip)?;

    Ok(())
}

fn write_firmware(segments: Vec<RomSegment>,
                  serial_port: Port,
                  port_info: UsbPortInfo,
                  chip: Chip) -> Result<(), Error> {
    info!("Connecting...");
    let mut flasher = Flasher::connect(
        *Box::new(serial_port),
        port_info,
        Some(115200),
        true,
        true,
        false,
        Some(chip),
        ResetAfterOperation::HardReset,
        ResetBeforeOperation::NoReset,
    )?;

    info!("Flashing...");
    flasher.write_bins_to_flash(segments.as_slice(), Some(&mut EspflashProgress::default()))
}

fn format_time(time: String) -> String {
    let parse1 = DateTime::parse_from_str(&time, "%Y-%m-%dT%H:%M:%S.%f%z").unwrap();
    parse1.format("%Y-%m-%d %H:%M:%S").to_string()
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct BuildInfo {
    commit: String,
    branch: String,
    commit_time: String,
    build_time: String,
}

/// Progress callback implementations for use in `cargo-espflash` and `espflash`
#[derive(Default)]
pub struct EspflashProgress {
    pb: Option<ProgressBar>,
}

impl ProgressCallbacks for EspflashProgress {
    /// Initialize the progress bar
    fn init(&mut self, addr: u32, len: usize) {
        let pb = ProgressBar::new(len as u64)
            .with_message(format!("{addr:#X}"))
            .with_style(
                ProgressStyle::default_bar()
                    .template("[{elapsed_precise}] [{bar:40}] {pos:>7}/{len:7} @ {msg}")
                    .unwrap()
                    .progress_chars("=> "),
            );

        self.pb = Some(pb);
    }

    /// Update the progress bar
    fn update(&mut self, current: usize) {
        if let Some(ref pb) = self.pb {
            pb.set_position(current as u64);
        }
    }

    /// End the progress bar
    fn finish(&mut self) {
        if let Some(ref pb) = self.pb {
            pb.finish();
        }
    }
}

/// Returns a vector with available USB serial ports.
fn detect_usb_serial_ports(list_all_ports: bool) -> Result<Vec<SerialPortInfo>> {
    let ports = available_ports().into_diagnostic()?;
    let ports = ports
        .into_iter()
        .filter(|port_info| {
            if list_all_ports {
                matches!(
                    &port_info.port_type,
                    SerialPortType::UsbPort(..) |
                    // Allow PciPort. The user may want to use it.
                    // The port might have been misdetected by the system as PCI.
                    SerialPortType::PciPort |
                    // Good luck.
                    SerialPortType::Unknown
                )
            } else {
                matches!(&port_info.port_type, SerialPortType::UsbPort(..))
            }
        })
        .collect::<Vec<_>>();

    Ok(ports)
}

fn select_device() -> Result<Device, Error> {
    let index = Select::with_theme(&ColorfulTheme::default())
        .item("Knomi V1 - DiverOfDark")
        .item("Knomi V2 - DiverOfDark")
        .item("Knomi V1 - BTT Original FW")
        .item("Knomi V2 - BTT Original FW")
        .default(0)
        .interact_opt()
        .map_err(|_| Error::Cancelled)?
        .ok_or(Error::Cancelled)?;

    match index {
        0 => Ok(Device::KnomiV1),
        1 => Ok(Device::KnomiV2),
        2 => Ok(Device::BttV1),
        3 => Ok(Device::BttV2),
        _ => Err(Error::Cancelled)
    }
}

/// Ask the user to select a serial port from a list of detected serial ports.
fn select_serial_port(ports: Vec<SerialPortInfo>) -> Result<SerialPortInfo, Error> {
    if ports.len() > 1 {
        // Multiple serial ports detected.
        info!("Detected {} serial ports", ports.len());
        info!("Please select a port");

        let port_names = ports
            .iter()
            .map(|port_info| {
                let formatted = port_info.port_name.as_str();
                match &port_info.port_type {
                    SerialPortType::UsbPort(info) => {
                        if let Some(product) = &info.product {
                            format!("{} - {}", formatted, product)
                        } else {
                            formatted.to_string()
                        }
                    }
                    _ => formatted.to_string(),
                }
            })
            .collect::<Vec<_>>();

        // https://github.com/console-rs/dialoguer/issues/77
        ctrlc::set_handler(move || {
            let term = dialoguer::console::Term::stdout();
            let _ = term.show_cursor();
        })
            .expect("Error setting Ctrl-C handler");

        let index = Select::with_theme(&ColorfulTheme::default())
            .items(&port_names)
            .default(0)
            .interact_opt()
            .map_err(|_| Error::Cancelled)?
            .ok_or(Error::Cancelled)?;

        match ports.get(index) {
            Some(port_info) => Ok(port_info.to_owned()),
            None => Err(Error::SerialNotFound(
                port_names.get(index).unwrap().to_string(),
            )),
        }
    } else if let [port] = ports.as_slice() {
        let port_name = port.port_name.clone();
        let product = match &port.port_type {
            SerialPortType::UsbPort(info) => info.product.as_ref(),
            _ => None,
        };
        if confirm_port(&port_name, product)? {
            Ok(port.to_owned())
        } else {
            Err(Error::Cancelled)
        }
    } else {
        // No serial ports detected
        Err(Error::NoSerial)
    }
}

fn confirm_flash() -> Result<(), Error> {
    let confirm = Confirm::with_theme(&ColorfulTheme::default())
        .with_prompt("Do you want to continue?")
        .default(false)
        .wait_for_newline(true)
        .interact()
        .map_err(|_| Error::Cancelled)?;

    if !confirm {
        return Err(Error::Cancelled);
    }
    Ok(())
}

/// Ask the user to confirm the use of a serial port.
fn confirm_port(port_name: &str, product: Option<&String>) -> Result<bool, Error> {
    Confirm::with_theme(&ColorfulTheme::default())
        .with_prompt({
            if let Some(product) = product {
                format!("Use serial port '{}' - {}?", port_name, product)
            } else {
                format!("Use serial port '{}'?", port_name)
            }
        })
        .show_default(true)
        .wait_for_newline(true)
        .interact_opt()
        .map_err(|_| Error::Cancelled)?
        .ok_or(Error::Cancelled)
}
