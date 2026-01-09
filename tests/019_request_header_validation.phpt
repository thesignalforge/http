--TEST--
signalforge_http: Request PSR-7 MessageInterface header validation (RFC 7230)
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up basic request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ACT: Test invalid header names (RFC 7230 validation)
$invalidHeaderNames = [
    "", // Empty
    "header with spaces", // Spaces
    "header:with:colons", // Colons
    "header\twith\tabs", // Control chars
    "header\nwith\nnewlines", // Newlines
    "header\rwith\rcarriage", // Carriage returns
    "header\x00with\x00nulls", // Null bytes
];

foreach ($invalidHeaderNames as $name) {
    try {
        $request->withHeader($name, 'value');
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test invalid header values (CRLF injection)
$invalidHeaderValues = [
    "value\r\ninjected: header", // CRLF injection
    "value\ninjected: header", // LF injection
    "value\rinjected: header", // CR injection
    "value\x00with\x00nulls", // Null bytes
];

foreach ($invalidHeaderValues as $value) {
    try {
        $request->withHeader('X-Test', $value);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test valid headers still work
$validRequest = $request->withHeader('X-Custom', 'valid value');
var_dump($validRequest->hasHeader('X-Custom'));
var_dump($validRequest->getHeader('X-Custom')[0] === 'valid value');

// ACT: Test getHeaderLine with empty headers
var_dump($request->getHeaderLine('X-Non-Existent') === '');

// ACT: Test withAddedHeader with empty arrays
$addedRequest = $request->withAddedHeader('X-Test', []);
var_dump(!$addedRequest->hasHeader('X-Test')); // Should not add empty array

// ACT: Test withAddedHeader with null values
try {
    $request->withAddedHeader('X-Test', null);
    var_dump(false); // Should not reach here
} catch (InvalidArgumentException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
