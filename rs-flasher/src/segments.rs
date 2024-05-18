use espflash::elf::RomSegment;
use espflash::targets::Chip;
use crate::segments::btt_v1_segments::get_btt_v1_segments;
use crate::segments::btt_v2_segments::get_btt_v2_segments;
use crate::segments::diverofdark_v1_segments::get_diverofdark_v1_segments;
use crate::segments::diverofdark_v2_segments::get_diverofdark_v2_segments;

mod utils;
mod diverofdark_v1_segments;
mod diverofdark_v2_segments;
mod btt_v1_segments;
mod btt_v2_segments;

pub enum Device {
    KnomiV1,
    KnomiV2,
    BttV1,
    BttV2
}
pub fn get_segments(device: &Device) -> Vec<RomSegment<'static>> {
    match device {
        Device::BttV1 => get_btt_v1_segments(),
        Device::BttV2 => get_btt_v2_segments(),
        Device::KnomiV1 => get_diverofdark_v1_segments(),
        Device::KnomiV2 => get_diverofdark_v2_segments()
    }
}

pub fn get_chip(device: &Device) -> Chip {
    match device {
        Device::BttV1 => Chip::Esp32,
        Device::BttV2 => Chip::Esp32s3,
        Device::KnomiV1 => Chip::Esp32,
        Device::KnomiV2 => Chip::Esp32s3,
    }
}
