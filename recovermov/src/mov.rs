const ATOM_TYPES: [&[u8; 4]; 24] = [
    b"ftyp", b"mdat", b"moov", b"pnot", b"udta", b"uuid", b"moof", b"free", b"skip", b"jP2 ",
    b"wide", b"load", b"ctab", b"imap", b"matt", b"kmat", b"clip", b"crgn", b"sync", b"chap",
    b"tmcd", b"scpt", b"ssrc", b"PICT",
];

pub fn is_valid_atom_type(atom_type: [u8; 4]) -> bool {
    ATOM_TYPES.iter().any(|t| atom_type == **t)
}
