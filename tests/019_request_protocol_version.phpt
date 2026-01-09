--TEST--
signalforge_http: Request PSR-7 MessageInterface protocol version edge cases
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

// ASSERT: Default protocol version
var_dump($request->getProtocolVersion() === '1.1');

// ACT: Set valid protocol versions (Request class only accepts HTTP/1.0 and HTTP/1.1)
$http10Request = $request->withProtocolVersion('1.0');
$http11Request = $request->withProtocolVersion('1.1');

// ASSERT: Valid protocol versions accepted
var_dump($http10Request->getProtocolVersion() === '1.0');
var_dump($http11Request->getProtocolVersion() === '1.1');

// ASSERT: Immutability maintained
var_dump($request->getProtocolVersion() === '1.1');

// ACT: Try invalid protocol versions (these should throw InvalidArgumentException)
$invalidVersions = ['', '0.9', '2.0', '2.1', '3.1', 'http/1.1', 'HTTP/1.1'];

foreach ($invalidVersions as $version) {
    try {
        $request->withProtocolVersion($version);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
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
