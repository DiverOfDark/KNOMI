use log::error;
use serde::Deserialize;


#[derive(Deserialize)]
pub struct CsvPartition {
    pub name: String,
    pub partition_type: String,
    pub sub_type: String,
    pub offset: String,
    pub size: String,
    pub flags: String
}

pub fn hex_str_to_u32(hex_str: &String) -> u32 {
    let stripped_hex = hex_str.trim().trim_start_matches("0x");
    let result = u32::from_str_radix(stripped_hex, 16);
    if result.is_err() {
        error!("{}", result.err().unwrap());
        panic!();
    }
    result.unwrap()
}

pub fn parse_partition_csv<F>(input: Vec<u8>, mut action: F)
    where
        F: FnMut(&CsvPartition) -> ()
{
    let mut reader = csv::ReaderBuilder::new().has_headers(false).comment(Some(b'#')).from_reader(input.as_slice());
    let headers = csv::StringRecord::from(vec!["name", "partition_type", "sub_type", "offset", "size", "flags"]);
    for record in reader.records() {
        if record.is_ok() {
            let record1 = record.unwrap();
            let result = record1.deserialize(Some(&headers));
            if result.is_ok() {
                let record: CsvPartition = result.unwrap();
                action(&record);
            }
        }
    }
}