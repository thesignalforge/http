--TEST--
signalforge_http: Response PSR-7 MessageInterface header validation (RFC 7230)
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ACT: Create response
$response = Response::create();

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
        $response->withHeader($name, 'value');
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
        $response->withHeader('X-Test', $value);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test valid headers still work
$validResponse = $response->withHeader('X-Custom', 'valid value');
var_dump($validResponse->hasHeader('X-Custom'));
var_dump($validResponse->getHeader('X-Custom')[0] === 'valid value');

// ACT: Test getHeaderLine with empty headers
var_dump($response->getHeaderLine('X-Non-Existent') === '');

// ACT: Test withAddedHeader with empty arrays
$addedResponse = $response->withAddedHeader('X-Test', []);
var_dump(!$addedResponse->hasHeader('X-Test')); // Should not add empty array

// ACT: Test withAddedHeader with null values
try {
    $response->withAddedHeader('X-Test', null);
    var_dump(false); // Should not reach here
} catch (InvalidArgumentException $e) {
    var_dump(true); // Exception correctly thrown
} catch (Exception $e) {
    var_dump(false); // Wrong exception type
}

// ACT: Test empty headers array handling
$emptyHeadersResponse = Response::create(200, []);
var_dump($emptyHeadersResponse->getHeaders() === []);

// ACT: Test headers with empty values array
$emptyValuesResponse = Response::create(200, ['X-Empty' => []]);
var_dump(!$emptyValuesResponse->hasHeader('X-Empty')); // Should not add header with empty values

// ACT: Test very long header values
$longValue = str_repeat('x', 8192); // 8KB header
$longHeaderResponse = $response->withHeader('X-Long', $longValue);
var_dump($longHeaderResponse->getHeader('X-Long')[0] === $longValue);

// ASSERT: Immutability maintained
var_dump($response->getHeaders() === []);
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
bool(true)
bool(true)
bool(true)
bool(true)
