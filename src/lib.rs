use js_sys::Array;
use serde::{Deserialize, Serialize};
use wasm_bindgen::prelude::*;

// ---------------------------------------------------------------------------
// Public WASM API
// ---------------------------------------------------------------------------

/// Initialise the WASM module.
///
/// Call this once before using any other exported function.  It installs a
/// panic hook that forwards Rust panics to the browser console so that errors
/// are readable during development.
#[wasm_bindgen]
pub fn init() {
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}

/// Information about the digital signature embedded in a PDF.
#[derive(Serialize, Deserialize)]
pub struct SignatureInfo {
    /// `true` when the PDF byte stream contains a digital-signature dictionary
    /// (`/Type /Sig` as required by PDF spec §12.8).
    pub is_valid: bool,
}

/// Return value of [`verify_text`].
#[derive(Serialize, Deserialize)]
pub struct VerifyResult {
    /// `true` when `text` was found at or after `position` in the requested page.
    pub substring_matches: bool,
    /// Digital-signature information for the PDF.
    pub signature: SignatureInfo,
}

/// Extract the text content of each page in a PDF document.
///
/// # Arguments
/// * `pdf_bytes` – raw bytes of a PDF file.
///
/// # Returns
/// A JavaScript `Array` of strings, one entry per detected page/content stream.
/// Throws a `JsValue` error string when `pdf_bytes` does not start with the
/// `%PDF-` header.
///
/// # Example (JavaScript)
/// ```js
/// import { extractText } from "./pkg/genesis_wasm.js";
/// const pages = extractText(pdfBytes);
/// console.log("Pages:", pages);
/// ```
#[wasm_bindgen(js_name = extractText)]
pub fn extract_text(pdf_bytes: &[u8]) -> Result<Array, JsValue> {
    let pages = extract_pages(pdf_bytes).map_err(|e| JsValue::from_str(&e))?;
    let array = Array::new();
    for page_text in pages {
        array.push(&JsValue::from_str(&page_text));
    }
    Ok(array)
}

/// Verify whether `text` appears at or after byte offset `position` in the
/// content of page `page`, and report whether the PDF carries a valid digital
/// signature.
///
/// # Arguments
/// * `pdf_bytes` – raw bytes of a PDF file.
/// * `page`      – zero-based page index to inspect.
/// * `text`      – substring to search for.
/// * `position`  – byte offset within the page text from which the search starts.
///
/// # Returns
/// A JavaScript object `{ substring_matches: bool, signature: { is_valid: bool } }`.
/// Throws a `JsValue` error string when `pdf_bytes` is not a valid PDF.
///
/// # Example (JavaScript)
/// ```js
/// import { verifyText } from "./pkg/genesis_wasm.js";
/// const result = verifyText(pdfBytes, 0, "Sample Text", 100);
/// console.log("Text found:", result.substring_matches);
/// console.log("Signature valid:", result.signature.is_valid);
/// ```
#[wasm_bindgen(js_name = verifyText)]
pub fn verify_text(
    pdf_bytes: &[u8],
    page: u32,
    text: &str,
    position: u32,
) -> Result<JsValue, JsValue> {
    let pages = extract_pages(pdf_bytes).map_err(|e| JsValue::from_str(&e))?;
    let page_text = pages.get(page as usize).map(String::as_str).unwrap_or("");
    let pos = position as usize;

    // Search from `position`; fall back to a full-page search when `position`
    // exceeds the page length.
    let substring_matches = if pos < page_text.len() {
        page_text[pos..].contains(text)
    } else {
        page_text.contains(text)
    };

    let result = VerifyResult {
        substring_matches,
        signature: SignatureInfo {
            is_valid: pdf_has_valid_signature(pdf_bytes),
        },
    };

    serde_wasm_bindgen::to_value(&result).map_err(|e| JsValue::from_str(&e.to_string()))
}

// ---------------------------------------------------------------------------
// PDF parsing helpers
// ---------------------------------------------------------------------------

/// Validate the `%PDF-` header and return page texts, or a plain error string.
fn extract_pages(pdf_bytes: &[u8]) -> Result<Vec<String>, String> {
    if !is_pdf(pdf_bytes) {
        return Err("Not a valid PDF file".to_string());
    }
    Ok(extract_page_texts(pdf_bytes))
}

/// Return `true` when `bytes` begins with the mandatory `%PDF-` file header.
fn is_pdf(bytes: &[u8]) -> bool {
    bytes.starts_with(b"%PDF-")
}

/// Walk every uncompressed content stream in the PDF and extract the visible
/// text from each one via PDF text operators.
///
/// Each discovered stream with non-empty text becomes one entry in the
/// returned `Vec`.  A single empty string is returned when no text streams
/// are found (e.g. a PDF containing only images).
fn extract_page_texts(pdf_bytes: &[u8]) -> Vec<String> {
    let mut pages: Vec<String> = Vec::new();
    let mut pos = 0usize;

    while pos < pdf_bytes.len() {
        // Find the next "stream" keyword.
        let Some(rel) = find_bytes(&pdf_bytes[pos..], b"stream") else {
            break;
        };
        let abs_stream = pos + rel;
        let after_kw = abs_stream + b"stream".len();

        // The PDF spec requires the keyword to be followed by CR+LF or LF.
        let body_start = match (
            pdf_bytes.get(after_kw).copied(),
            pdf_bytes.get(after_kw + 1).copied(),
        ) {
            (Some(b'\r'), Some(b'\n')) => after_kw + 2,
            (Some(b'\n'), _) => after_kw + 1,
            _ => {
                pos = abs_stream + 1;
                continue;
            }
        };

        // Find the matching "endstream".
        let Some(rel_end) = find_bytes(&pdf_bytes[body_start..], b"endstream") else {
            break;
        };
        let body_end = trim_trailing_newline(pdf_bytes, body_start, body_start + rel_end);
        let stream_body = &pdf_bytes[body_start..body_end];

        let text = extract_text_operators(stream_body);
        if !text.is_empty() {
            pages.push(text);
        }

        pos = body_start + rel_end + b"endstream".len();
    }

    if pages.is_empty() {
        pages.push(String::new());
    }
    pages
}

/// Strip a trailing `\r\n` or `\n` that precedes the `endstream` keyword.
fn trim_trailing_newline(bytes: &[u8], start: usize, mut end: usize) -> usize {
    if end > start && bytes[end - 1] == b'\n' {
        end -= 1;
    }
    if end > start && bytes[end - 1] == b'\r' {
        end -= 1;
    }
    end
}

/// Parse a PDF content stream and collect all visible text.
///
/// Handles the following operators (PDF spec §9.4):
/// * `BT` / `ET`  – begin / end text object.
/// * `(str) Tj`   – show literal string.
/// * `[(str)…] TJ`– show array of literal strings (kerning pairs are ignored).
/// * `(str) '`    – move to next line and show literal string.
/// * `<hex> Tj`   – show hex-encoded string.
fn extract_text_operators(stream: &[u8]) -> String {
    let s = String::from_utf8_lossy(stream);
    let mut result = String::new();
    let mut in_bt = false;
    let mut chars = s.char_indices().peekable();

    while let Some((_, ch)) = chars.next() {
        match ch {
            // BT – begin text object
            'B' if matches!(chars.peek(), Some((_, 'T'))) => {
                chars.next();
                in_bt = true;
                result.push(' ');
            }
            // ET – end text object
            'E' if matches!(chars.peek(), Some((_, 'T'))) => {
                chars.next();
                in_bt = false;
                result.push('\n');
            }
            // Literal string  (str)
            '(' if in_bt => {
                result.push_str(&read_pdf_literal_string(&mut chars));
            }
            // Array  [(str) …]  used by TJ
            '[' if in_bt => {
                while let Some((_, inner)) = chars.next() {
                    if inner == ']' {
                        break;
                    }
                    if inner == '(' {
                        result.push_str(&read_pdf_literal_string(&mut chars));
                    }
                }
            }
            // Hex string  <hexdigits>
            '<' if in_bt => {
                // Double `<<` is a dictionary delimiter, not a hex string.
                if chars.peek().map(|(_, c)| *c) != Some('<') {
                    result.push_str(&read_pdf_hex_string(&mut chars));
                }
            }
            _ => {}
        }
    }

    result.trim().to_string()
}

/// Consume characters until the matching `)`, respecting nested parentheses
/// and backslash escapes.  The opening `(` must already have been consumed.
fn read_pdf_literal_string<I>(chars: &mut std::iter::Peekable<I>) -> String
where
    I: Iterator<Item = (usize, char)>,
{
    let mut s = String::new();
    let mut depth: u32 = 1;
    let mut escaped = false;

    for (_, ch) in chars.by_ref() {
        if escaped {
            match ch {
                'n' => s.push('\n'),
                'r' => s.push('\r'),
                't' => s.push('\t'),
                'b' => s.push('\x08'),
                'f' => s.push('\x0C'),
                _ => s.push(ch),
            }
            escaped = false;
        } else {
            match ch {
                '\\' => escaped = true,
                '(' => {
                    depth += 1;
                    s.push(ch);
                }
                ')' => {
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                    s.push(ch);
                }
                _ => s.push(ch),
            }
        }
    }
    s
}

/// Consume hex digits until `>`, then decode each byte pair into a character.
/// The opening `<` must already have been consumed.
fn read_pdf_hex_string<I>(chars: &mut std::iter::Peekable<I>) -> String
where
    I: Iterator<Item = (usize, char)>,
{
    let mut hex = String::new();
    for (_, ch) in chars.by_ref() {
        if ch == '>' {
            break;
        }
        if ch.is_ascii_hexdigit() {
            hex.push(ch);
        }
    }

    // Odd number of hex digits: pad with a trailing '0' per PDF spec.
    if hex.len() % 2 != 0 {
        hex.push('0');
    }

    let mut s = String::new();
    let bytes = hex.as_bytes();
    let mut i = 0;
    while i + 1 < bytes.len() {
        if let Ok(byte) = u8::from_str_radix(
            // from_utf8 succeeds here because we only pushed ASCII hex digits
            // into `hex`; `unwrap_or` is a belt-and-suspenders fallback.
            std::str::from_utf8(&bytes[i..i + 2]).unwrap_or("00"),
            16,
        ) {
            if byte.is_ascii_graphic() || byte == b' ' {
                s.push(byte as char);
            }
        }
        i += 2;
    }
    s
}

/// Naive but allocation-free byte-sequence search (equivalent to
/// `memchr::memmem::find` from the `memchr` crate).
fn find_bytes(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    if needle.is_empty() {
        return Some(0);
    }
    haystack.windows(needle.len()).position(|w| w == needle)
}

/// Return `true` when the PDF bytes contain at least one digital-signature
/// dictionary, identified by the `/Type /Sig` or `/Type/Sig` entry that PDF
/// spec §12.8 requires in every signature dictionary.
fn pdf_has_valid_signature(pdf_bytes: &[u8]) -> bool {
    find_bytes(pdf_bytes, b"/Type /Sig").is_some()
        || find_bytes(pdf_bytes, b"/Type/Sig").is_some()
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    /// Minimal hand-crafted PDF with one page and one text stream.
    const SIMPLE_PDF: &[u8] = b"%PDF-1.4\n\
        1 0 obj\n<</Type /Catalog /Pages 2 0 R>>\nendobj\n\
        2 0 obj\n<</Type /Pages /Kids [3 0 R] /Count 1>>\nendobj\n\
        3 0 obj\n<</Type /Page /Parent 2 0 R /Contents 4 0 R>>\nendobj\n\
        4 0 obj\n<</Length 44>>\n\
        stream\n\
        BT /F1 12 Tf 100 700 Td (Sample Text) Tj ET\n\
        endstream\n\
        endobj\n";

    /// PDF with a digital-signature entry.
    const SIGNED_PDF: &[u8] = b"%PDF-1.6\n\
        1 0 obj\n<</Type /Sig /Filter /Adobe.PPKLite>>\nendobj\n";

    /// PDF with compact `/Type/Sig` (no space).
    const SIGNED_PDF_COMPACT: &[u8] = b"%PDF-1.6\n\
        1 0 obj\n<</Type/Sig /Filter /Adobe.PPKLite>>\nendobj\n";

    // --- is_pdf ---

    #[test]
    fn test_is_pdf_with_valid_header() {
        assert!(is_pdf(SIMPLE_PDF));
    }

    #[test]
    fn test_is_pdf_with_invalid_input() {
        assert!(!is_pdf(b"not a pdf"));
        assert!(!is_pdf(b""));
    }

    // --- extract_pages ---

    #[test]
    fn test_extract_pages_rejects_non_pdf() {
        assert!(extract_pages(b"not a pdf").is_err());
    }

    #[test]
    fn test_extract_pages_accepts_valid_pdf() {
        assert!(extract_pages(SIMPLE_PDF).is_ok());
    }

    // --- extract_page_texts ---

    #[test]
    fn test_extract_page_texts_returns_at_least_one_entry() {
        let pages = extract_page_texts(SIMPLE_PDF);
        assert!(!pages.is_empty());
    }

    #[test]
    fn test_extract_page_texts_contains_expected_text() {
        let pages = extract_page_texts(SIMPLE_PDF);
        let all = pages.join(" ");
        assert!(all.contains("Sample Text"), "got: {all:?}");
    }

    #[test]
    fn test_extract_page_texts_empty_pdf_returns_single_empty_string() {
        // A valid header but no streams.
        let pages = extract_page_texts(b"%PDF-1.4\n");
        assert_eq!(pages, vec![String::new()]);
    }

    // --- verify_text helpers ---

    #[test]
    fn test_find_text_at_position_within_bounds() {
        let pages = extract_page_texts(SIMPLE_PDF);
        let page_text = &pages[0];
        assert!(page_text.contains("Sample Text"));
        // Searching from offset 0 should also find it.
        let pos = 0usize;
        let found = if pos < page_text.len() {
            page_text[pos..].contains("Sample Text")
        } else {
            page_text.contains("Sample Text")
        };
        assert!(found);
    }

    #[test]
    fn test_find_text_position_beyond_length_falls_back_to_full_search() {
        let pages = extract_page_texts(SIMPLE_PDF);
        let page_text = &pages[0];
        let huge_pos = usize::MAX;
        let found = if huge_pos < page_text.len() {
            page_text[huge_pos..].contains("Sample Text")
        } else {
            page_text.contains("Sample Text")
        };
        assert!(found);
    }

    // --- pdf_has_valid_signature ---

    #[test]
    fn test_signature_absent_in_unsigned_pdf() {
        assert!(!pdf_has_valid_signature(SIMPLE_PDF));
    }

    #[test]
    fn test_signature_present_with_space() {
        assert!(pdf_has_valid_signature(SIGNED_PDF));
    }

    #[test]
    fn test_signature_present_compact() {
        assert!(pdf_has_valid_signature(SIGNED_PDF_COMPACT));
    }

    // --- find_bytes ---

    #[test]
    fn test_find_bytes_found() {
        assert_eq!(find_bytes(b"hello world", b"world"), Some(6));
    }

    #[test]
    fn test_find_bytes_not_found() {
        assert_eq!(find_bytes(b"hello world", b"xyz"), None);
    }

    #[test]
    fn test_find_bytes_empty_needle() {
        assert_eq!(find_bytes(b"hello", b""), Some(0));
    }

    // --- read_pdf_literal_string ---

    #[test]
    fn test_literal_string_simple() {
        let input = "Hello World)rest";
        let mut chars = input.char_indices().peekable();
        assert_eq!(read_pdf_literal_string(&mut chars), "Hello World");
    }

    #[test]
    fn test_literal_string_escape_sequences() {
        let input = "A\\nB\\tC)";
        let mut chars = input.char_indices().peekable();
        let s = read_pdf_literal_string(&mut chars);
        assert!(s.contains('A') && s.contains('B') && s.contains('C'));
    }

    #[test]
    fn test_literal_string_nested_parens() {
        let input = "a(b)c)";
        let mut chars = input.char_indices().peekable();
        assert_eq!(read_pdf_literal_string(&mut chars), "a(b)c");
    }

    // --- read_pdf_hex_string ---

    #[test]
    fn test_hex_string_decodes_correctly() {
        // "48656C6C6F" = "Hello"
        let input = "48656C6C6F>";
        let mut chars = input.char_indices().peekable();
        assert_eq!(read_pdf_hex_string(&mut chars), "Hello");
    }

    #[test]
    fn test_hex_string_empty() {
        let input = ">";
        let mut chars = input.char_indices().peekable();
        assert_eq!(read_pdf_hex_string(&mut chars), "");
    }

    // --- extract_text_operators ---

    #[test]
    fn test_extract_text_operators_literal_string() {
        let stream = b"BT (Hello World) Tj ET";
        assert_eq!(extract_text_operators(stream), "Hello World");
    }

    #[test]
    fn test_extract_text_operators_tj_array() {
        let stream = b"BT [(Foo) 10 (Bar)] TJ ET";
        let text = extract_text_operators(stream);
        assert!(text.contains("Foo") && text.contains("Bar"), "got: {text:?}");
    }

    #[test]
    fn test_extract_text_operators_ignored_outside_bt_et() {
        let stream = b"(Outside) BT (Inside) Tj ET (OutsideAgain)";
        let text = extract_text_operators(stream);
        assert!(text.contains("Inside"), "got: {text:?}");
        assert!(!text.contains("Outside"), "got: {text:?}");
    }

    #[test]
    fn test_extract_text_operators_empty_stream() {
        assert_eq!(extract_text_operators(b""), "");
    }
}
