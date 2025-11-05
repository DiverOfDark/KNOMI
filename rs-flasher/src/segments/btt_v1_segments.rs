use espflash::image_format::Segment;
use include_bytes_zstd::include_bytes_zstd;
use std::borrow::Cow;

pub fn get_btt_v1_segments() -> Vec<Segment<'static>> {
    let boot_ota = include_bytes_zstd!("static/boot_app0.bin", 19);
    let bootloader = include_bytes_zstd!("static/btt_v1/knomi1_bootloader.bin", 19);
    let firmware = include_bytes_zstd!("static/btt_v1/knomi1_firmware.bin", 19);
    let partitions = include_bytes_zstd!("static/btt_v1/knomi1_partitions.bin", 19);

    let mut segments = Vec::new();

    segments.push(Segment {
        addr: 0x1000,
        data: Cow::Owned(bootloader),
    });
    segments.push(Segment {
        addr: 0x8000,
        data: Cow::Owned(partitions),
    });
    segments.push(Segment {
        addr: 0xe000,
        data: Cow::Owned(boot_ota),
    });
    segments.push(Segment {
        addr: 0x10000,
        data: Cow::Owned(firmware),
    });
    segments
}
