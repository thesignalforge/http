--TEST--
signalforge_http: Request PSR-7 RequestInterface URI and request target edge cases
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

// ASSERT: Default URI
var_dump($request->getUri() === '/test');

// ACT: Test getRequestTarget with various scenarios
var_dump($request->getRequestTarget() === '/test');

// ACT: Test withRequestTarget with asterisk form
$asteriskRequest = $request->withRequestTarget('*');
var_dump($asteriskRequest->getRequestTarget() === '*');

// ACT: Test withRequestTarget with absolute-form
$absoluteRequest = $request->withRequestTarget('http://example.com/path');
var_dump($absoluteRequest->getRequestTarget() === 'http://example.com/path');

// ACT: Test withRequestTarget with empty string
$emptyRequest = $request->withRequestTarget('');
var_dump($emptyRequest->getRequestTarget() === '');

// ACT: Test withUri with various URI formats
$uriRequest1 = $request->withUri('/simple/path');
var_dump($uriRequest1->getUri() === '/simple/path');

$uriRequest2 = $request->withUri('/path?query=value&other=123');
var_dump($uriRequest2->getUri() === '/path?query=value&other=123');

$uriRequest3 = $request->withUri('https://example.com:8080/path#fragment');
var_dump($uriRequest3->getUri() === 'https://example.com:8080/path#fragment');

// ACT: Test withUri with invalid URIs
$invalidUris = [
    null,
    123,
    [],
    new stdClass(),
];

foreach ($invalidUris as $uri) {
    try {
        $request->withUri($uri);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test withUri preserveHost flag behavior
$originalHostRequest = $request->withHeader('Host', 'original.com');
$preserveHostRequest = $originalHostRequest->withUri('https://new.com/path', true);
$notPreserveHostRequest = $originalHostRequest->withUri('https://new.com/path', false);

// ASSERT: Host header behavior with preserveHost flag
var_dump($preserveHostRequest->getHeader('Host')[0] === 'original.com');
var_dump($notPreserveHostRequest->getHeader('Host')[0] === 'new.com');

// ASSERT: Immutability maintained
var_dump($request->getUri() === '/test');
var_dump($request->getRequestTarget() === '/test');
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
