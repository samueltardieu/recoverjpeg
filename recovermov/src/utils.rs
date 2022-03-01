pub fn with_suffix(s: &str, default_multiplier: u64) -> Result<u64, String> {
    u64_suffix(s, default_multiplier).ok_or(format!("invalid size {s}"))
}

pub fn bytes_with_suffix(s: &str) -> Result<u64, String> {
    with_suffix(s, 1)
}

pub fn mib_with_suffix(s: &str) -> Result<u64, String> {
    with_suffix(s, 1024 * 1024)
}

fn u64_suffix(s: &str, default_multiplier: u64) -> Option<u64> {
    let mut has_suffix = false;
    let ls = s.to_ascii_lowercase();
    let (mut ls, k) = if let Some(ls) = ls.strip_suffix("ib") {
        has_suffix = true;
        (ls, 1024)
    } else if let Some(ls) = ls.strip_suffix('b') {
        has_suffix = true;
        (ls, 1000)
    } else {
        (ls.as_str(), 1000)
    };
    let (last_idx, suffix) = ls.char_indices().last()?;
    has_suffix |= !suffix.is_numeric();
    let multiplier = match suffix {
        'k' | 'K' => k,
        'm' | 'M' => k * k,
        'g' | 'G' => k * k * k,
        't' | 'T' => k * k * k * k,
        '0'..='9' if k == 1000 && !has_suffix => default_multiplier,
        '0'..='9' if k == 1000 => 1,
        _ => return None,
    };
    if !suffix.is_numeric() {
        ls = &ls[..last_idx];
    };
    match ls.parse::<u64>() {
        Ok(v) => Some(v * multiplier),
        Err(_) => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_with_suffix() {
        assert_eq!(Ok(127), bytes_with_suffix("127"));
        assert_eq!(Ok(127), bytes_with_suffix("127b"));
        assert_eq!(Ok(127), bytes_with_suffix("127B"));
        assert!(bytes_with_suffix("127ib").is_err());
        assert_eq!(Ok(133_169_152), mib_with_suffix("127"));
        assert_eq!(Ok(127), mib_with_suffix("127b"));
        assert_eq!(Ok(127), mib_with_suffix("127B"));
        assert!(mib_with_suffix("127ib").is_err());
        assert_eq!(Ok(127_000), with_suffix("127kb", 1));
        assert_eq!(Ok(130_048), with_suffix("127kib", 1));
        assert_eq!(Ok(127_000), with_suffix("127kb", 1000));
        assert_eq!(Ok(130_048), with_suffix("127kib", 1000));
        assert_eq!(Ok(27_000_000_000_000), with_suffix("27TB", 1_000_000));
        assert_eq!(Ok(29_686_813_949_952), with_suffix("27TiB", 1_000_000));
    }
}
