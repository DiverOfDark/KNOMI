use std::borrow::Cow;
use espflash::elf::RomSegment;
use include_bytes_zstd::include_bytes_zstd;

pub fn get_btt_v2_segments() -> Vec<RomSegment<'static>> {
    let boot_ota = include_bytes_zstd!("static/boot_app0.bin", 19);
    let bootloader = include_bytes_zstd!("static/btt_v2/knomi2_bootloader.bin", 19);
    let firmware = include_bytes_zstd!("static/btt_v2/knomi2_firmware.bin", 19);
    let partitions = include_bytes_zstd!("static/btt_v2/knomi2_partitions.bin", 19);

    let mut segments = Vec::new();

    segments.push(RomSegment { addr: 0x0000, data: Cow::Owned(bootloader) });
    segments.push(RomSegment { addr: 0x8000, data: Cow::Owned(partitions) });
    segments.push(RomSegment { addr: 0xe000, data: Cow::Owned(boot_ota) });
    segments.push(RomSegment { addr: 0x10000, data: Cow::Owned(firmware) });
    segments
}