/*
 * test_TextDecoderBOMEncoding.js
 * bug 764234 tests
*/

function runTextDecoderBOMEnoding()
{
  test(testDecodeValidBOMUTF16, "testDecodeValidBOMUTF16");
  test(testBOMEncodingUTF8, "testBOMEncodingUTF8");
  test(testMoreBOMEncoding, "testMoreBOMEncoding");
}

function testDecodeValidBOMUTF16() {
  var expectedString = "\"\u0412\u0441\u0435 \u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u044B\u0435 \u0441\u0435\u043C\u044C\u0438 \u043F\u043E\u0445\u043E\u0436\u0438 \u0434\u0440\u0443\u0433 \u043D\u0430 \u0434\u0440\u0443\u0433\u0430, \u043A\u0430\u0436\u0434\u0430\u044F \u043D\u0435\u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u0430\u044F \u0441\u0435\u043C\u044C\u044F \u043D\u0435\u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u0430 \u043F\u043E-\u0441\u0432\u043E\u0435\u043C\u0443.\"";

  // Testing UTF-16BE
  var data = [0xFE, 0xFF, 0x00, 0x22, 0x04, 0x12, 0x04, 0x41, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x4B, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x38, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x04, 0x45, 0x04, 0x3E, 0x04, 0x36, 0x04, 0x38, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x30, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x30, 0x00, 0x2C, 0x00, 0x20, 0x04, 0x3A, 0x04, 0x30, 0x04, 0x36, 0x04, 0x34, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x00, 0x2D, 0x04, 0x41, 0x04, 0x32, 0x04, 0x3E, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x43, 0x00, 0x2E, 0x00, 0x22];
  testBOMCharset({encoding: "utf-16be", data: data, expected: expectedString,
                  msg: "decoder valid UTF-16BE test."});
}

function testBOMEncodingUTF8() {

  // basic utf-8 test with valid encoding and byte stream. no byte om provided.
  var data = [0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27];
  var expectedString = " !\"#$%&'";
  testBOMCharset({encoding: "utf-8", data: data, expected: expectedString,
                  msg: "utf-8 encoding."});

  // test valid encoding provided with valid byte OM also provided.
  data = [0xEF, 0xBB, 0xBF, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27];
  expectedString = " !\"#$%&'";
  testBOMCharset({encoding: "utf-8", data: data, expected: expectedString,
                  msg: "valid utf-8 encoding provided with VALID utf-8 BOM test."});

  // test valid encoding provided with invalid byte OM also provided.
  data = [0xFF, 0xFE, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27];
  testBOMCharset({encoding: "utf-8", fatal: true, data: data, error: "EncodingError",
                  msg: "valid utf-8 encoding provided with invalid utf-8 fatal BOM test."});

  // test valid encoding provided with invalid byte OM also provided.
  data = [0xFF, 0xFE, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27];
  expectedString = "\ufffd\ufffd !\"#$%&'";
  testBOMCharset({encoding: "utf-8", data: data, expected: expectedString,
                  msg: "valid utf-8 encoding provided with invalid utf-8 BOM test."});

  // test empty encoding provided with invalid byte OM also provided.
  data = [0xFF, 0xFE, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27];
  testBOMCharset({encoding: "", data: data, error: "EncodingError",
                  msg: "empty encoding provided with invalid utf-8 BOM test."});
}

function testMoreBOMEncoding() {
  var expectedString = "\"\u0412\u0441\u0435 \u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u044B\u0435 \u0441\u0435\u043C\u044C\u0438 \u043F\u043E\u0445\u043E\u0436\u0438 \u0434\u0440\u0443\u0433 \u043D\u0430 \u0434\u0440\u0443\u0433\u0430, \u043A\u0430\u0436\u0434\u0430\u044F \u043D\u0435\u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u0430\u044F \u0441\u0435\u043C\u044C\u044F \u043D\u0435\u0441\u0447\u0430\u0441\u0442\u043B\u0438\u0432\u0430 \u043F\u043E-\u0441\u0432\u043E\u0435\u043C\u0443.\"";

 // Testing user provided encoding is UTF-16BE & bom encoding is utf-16le
  var data = [0xFF, 0xFE, 0x00, 0x22, 0x04, 0x12, 0x04, 0x41, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x4B, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x38, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x04, 0x45, 0x04, 0x3E, 0x04, 0x36, 0x04, 0x38, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x30, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x30, 0x00, 0x2C, 0x00, 0x20, 0x04, 0x3A, 0x04, 0x30, 0x04, 0x36, 0x04, 0x34, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x00, 0x2D, 0x04, 0x41, 0x04, 0x32, 0x04, 0x3E, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x43, 0x00, 0x2E, 0x00, 0x22];

  testBOMCharset({encoding: "utf-16be", fatal: true, data: data, expected: "\ufffe" + expectedString,
                  msg: "test decoder invalid BOM encoding for utf-16be fatal."});

  testBOMCharset({encoding: "utf-16be", data: data, expected: "\ufffe" + expectedString,
                  msg: "test decoder invalid BOM encoding for utf-16be."});

  // Testing user provided encoding is UTF-16LE & bom encoding is utf-16be
  var dataUTF16 = [0xFE, 0xFF, 0x22, 0x00, 0x12, 0x04, 0x41, 0x04, 0x35, 0x04, 0x20, 0x00, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x4B, 0x04, 0x35, 0x04, 0x20, 0x00, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x38, 0x04, 0x20, 0x00, 0x3F, 0x04, 0x3E, 0x04, 0x45, 0x04, 0x3E, 0x04, 0x36, 0x04, 0x38, 0x04, 0x20, 0x00, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x30, 0x04, 0x20, 0x00, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x30, 0x04, 0x2C, 0x00, 0x20, 0x00, 0x3A, 0x04, 0x30, 0x04, 0x36, 0x04, 0x34, 0x04, 0x30, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x20, 0x00, 0x3F, 0x04, 0x3E, 0x04, 0x2D, 0x00, 0x41, 0x04, 0x32, 0x04, 0x3E, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x43, 0x04, 0x2E, 0x00, 0x22, 0x00];
  testBOMCharset({encoding: "utf-16le", fatal: true, data: dataUTF16, expected: "\ufffe" + expectedString,
                  msg: "test decoder invalid BOM encoding for utf-16 fatal."});

  testBOMCharset({encoding: "utf-16le", data: dataUTF16, expected: "\ufffe" + expectedString,
                  msg: "test decoder invalid BOM encoding for utf-16."});

  // Testing user provided encoding is UTF-16 & bom encoding is utf-16be
  data = [0xFE, 0xFF, 0x00, 0x22, 0x04, 0x12, 0x04, 0x41, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x4B, 0x04, 0x35, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x38, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x04, 0x45, 0x04, 0x3E, 0x04, 0x36, 0x04, 0x38, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x30, 0x00, 0x20, 0x04, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x30, 0x00, 0x2C, 0x00, 0x20, 0x04, 0x3A, 0x04, 0x30, 0x04, 0x36, 0x04, 0x34, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x4F, 0x00, 0x20, 0x04, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x00, 0x20, 0x04, 0x3F, 0x04, 0x3E, 0x00, 0x2D, 0x04, 0x41, 0x04, 0x32, 0x04, 0x3E, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x43, 0x00, 0x2E, 0x00, 0x22];
  testBOMCharset({encoding: "utf-16", fatal: true, data: data, expected: expectedString,
                  msg: "test decoder BOM encoding for utf-16 fatal."});

  testBOMCharset({encoding: "utf-16", data: data, expected: expectedString,
                  msg: "test decoder BOM encoding for utf-16."});

  // Testing user provided encoding is UTF-16 & bom encoding is utf-16le
  dataUTF16 = [0xFF, 0xFE, 0x22, 0x00, 0x12, 0x04, 0x41, 0x04, 0x35, 0x04, 0x20, 0x00, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x4B, 0x04, 0x35, 0x04, 0x20, 0x00, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x38, 0x04, 0x20, 0x00, 0x3F, 0x04, 0x3E, 0x04, 0x45, 0x04, 0x3E, 0x04, 0x36, 0x04, 0x38, 0x04, 0x20, 0x00, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x30, 0x04, 0x20, 0x00, 0x34, 0x04, 0x40, 0x04, 0x43, 0x04, 0x33, 0x04, 0x30, 0x04, 0x2C, 0x00, 0x20, 0x00, 0x3A, 0x04, 0x30, 0x04, 0x36, 0x04, 0x34, 0x04, 0x30, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x41, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x4C, 0x04, 0x4F, 0x04, 0x20, 0x00, 0x3D, 0x04, 0x35, 0x04, 0x41, 0x04, 0x47, 0x04, 0x30, 0x04, 0x41, 0x04, 0x42, 0x04, 0x3B, 0x04, 0x38, 0x04, 0x32, 0x04, 0x30, 0x04, 0x20, 0x00, 0x3F, 0x04, 0x3E, 0x04, 0x2D, 0x00, 0x41, 0x04, 0x32, 0x04, 0x3E, 0x04, 0x35, 0x04, 0x3C, 0x04, 0x43, 0x04, 0x2E, 0x00, 0x22, 0x00];
  testBOMCharset({encoding: "utf-16", fatal: true, data: dataUTF16, expected: expectedString,
                  msg: "test decoder BOM encoding for utf-16 fatal."});

  testBOMCharset({encoding: "utf-16", data: dataUTF16, expected: expectedString,
                  msg: "test decoder BOM encoding for utf-16."});

  // Testing user provided encoding is UTF-8 & bom encoding is utf-16be
  data = [0xFE, 0xFF, 0x22, 0xd0, 0x92, 0xd1, 0x81, 0xd0, 0xb5, 0x20, 0xd1, 0x81, 0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbb, 0xd0, 0xb8, 0xd0, 0xb2, 0xd1, 0x8b, 0xd0, 0xb5, 0x20, 0xd1, 0x81, 0xd0, 0xb5, 0xd0, 0xbc, 0xd1, 0x8c, 0xd0, 0xb8, 0x20, 0xd0, 0xbf, 0xd0, 0xbe, 0xd1, 0x85, 0xd0, 0xbe, 0xd0, 0xb6, 0xd0, 0xb8, 0x20, 0xd0, 0xb4, 0xd1, 0x80, 0xd1, 0x83, 0xd0, 0xb3, 0x20, 0xd0, 0xbd, 0xd0, 0xb0, 0x20, 0xd0, 0xb4, 0xd1, 0x80, 0xd1, 0x83, 0xd0, 0xb3, 0xd0, 0xb0, 0x2c, 0x20, 0xd0, 0xba, 0xd0, 0xb0, 0xd0, 0xb6, 0xd0, 0xb4, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd0, 0xbd, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbb, 0xd0, 0xb8, 0xd0, 0xb2, 0xd0, 0xb0, 0xd1, 0x8f, 0x20, 0xd1, 0x81, 0xd0, 0xb5, 0xd0, 0xbc, 0xd1, 0x8c, 0xd1, 0x8f, 0x20, 0xd0, 0xbd, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbb, 0xd0, 0xb8, 0xd0, 0xb2, 0xd0, 0xb0, 0x20, 0xd0, 0xbf, 0xd0, 0xbe, 0x2d, 0xd1, 0x81, 0xd0, 0xb2, 0xd0, 0xbe, 0xd0, 0xb5, 0xd0, 0xbc, 0xd1, 0x83, 0x2e, 0x22];

  testBOMCharset({encoding: "utf-8", fatal: true, data: data, error: "EncodingError",
                  msg: "test decoder invalid BOM encoding for valid utf-8 fatal provided label."});

  testBOMCharset({encoding: "utf-8", data: data, expected: "\ufffd\ufffd" + expectedString,
                  msg: "test decoder invalid BOM encoding for valid utf-8 provided label."});

  // Testing user provided encoding is non-UTF & bom encoding is utf-16be
  data = [0xFE, 0xFF, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe];

  expectedString = "\u03CE\uFFFD\u2019\xA3\u20AC\u20AF\xA6\xA7\xA8\xA9\u037A\xAB\xAC\xAD\u2015"
                 + "\xB0\xB1\xB2\xB3\u0384\u0385\u0386\xB7\u0388\u0389\u038A\xBB\u038C\xBD\u038E\u038F"
                 + "\u0390\u0391\u0392\u0393\u0394\u0395\u0396\u0397\u0398\u0399\u039A\u039B\u039C\u039D\u039E\u039F"
                 + "\u03A0\u03A1\u03A3\u03A4\u03A5\u03A6\u03A7\u03A8\u03A9\u03AA\u03AB\u03AC\u03AD\u03AE\u03AF"
                 + "\u03B0\u03B1\u03B2\u03B3\u03B4\u03B5\u03B6\u03B7\u03B8\u03B9\u03BA\u03BB\u03BC\u03BD\u03BE\u03BF"
                 + "\u03C0\u03C1\u03C2\u03C3\u03C4\u03C5\u03C6\u03C7\u03C8\u03C9\u03CA\u03CB\u03CC\u03CD\u03CE";

  testBOMCharset({encoding: "greek", fatal: true, data: data, error: "EncodingError",
                  msg: "test decoder encoding provided with invalid BOM encoding for greek."});

  testBOMCharset({encoding: "greek", data: data, expected: expectedString,
                  msg: "test decoder encoding provided with invalid BOM encoding for greek."});
}

function testBOMCharset(test)
{
  var outText;
  try {
    var decoder = 'fatal' in test ?
      TextDecoder(test.encoding, {fatal: test.fatal}) :
      TextDecoder(test.encoding);
    outText = decoder.decode(new Uint8Array(test.data));
  } catch (e) {
    assert_equals(e.name, test.error, test.msg);
    return;
  }
  assert_true(!test.error, test.msg);

  if (outText !== test.expected) {
    assert_equals(escape(outText), escape(test.expected), test.msg + " Code points do not match expected code points.");
  }
}

