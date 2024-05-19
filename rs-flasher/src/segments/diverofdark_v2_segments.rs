use std::borrow::Cow;
use espflash::elf::RomSegment;
use include_bytes_zstd::include_bytes_zstd;
use crate::segments::utils::{hex_str_to_u32, parse_partition_csv};

pub fn get_diverofdark_v2_segments() -> Vec<RomSegment<'static>> {
    let boot_ota = include_bytes_zstd!("static/boot_app0.bin", 19);
    let bootloader = include_bytes_zstd!("resources/knomiv2/bootloader.bin", 19);
    let firmware = include_bytes_zstd!("resources/knomiv2/firmware.bin", 19);
    let partitions = include_bytes_zstd!("resources/knomiv2/partitions.bin", 19);
    let csv = include_bytes_zstd!("resources/knomiv2/partition.csv", 19);
    let littlefs = include_bytes_zstd!("resources/knomiv2/littlefs.bin", 19);

    let mut segments = Vec::new();

    segments.push(RomSegment { addr: 0x0000, data: Cow::Owned(bootloader) });
    segments.push(RomSegment { addr: 0x8000, data: Cow::Owned(partitions) });

    parse_partition_csv(csv, |record| {
        if record.name.trim() == "otadata" && record.sub_type.trim() == "ota" {
            segments.push(RomSegment { addr: hex_str_to_u32(&record.offset), data: Cow::Owned(boot_ota.clone()) });
        }
        if record.name.trim() == "app0" && record.sub_type.trim() == "ota_0" {
            segments.push(RomSegment { addr: hex_str_to_u32(&record.offset), data: Cow::Owned(firmware.clone()) });
        }
        if record.name.trim() == "app1" && record.sub_type.trim() == "ota_1" {
            segments.push(RomSegment { addr: hex_str_to_u32(&record.offset), data: Cow::Owned(firmware.clone()) });
        }
        if record.name.trim() == "spiffs" && record.sub_type.trim() == "spiffs" {
            segments.push(RomSegment { addr: hex_str_to_u32(&record.offset), data: Cow::Owned(littlefs.clone()) });
        }
    });

    segments
}