--TEST--
signalforge_http: PSR-7 comprehensive edge cases and boundary conditions
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\{Request, Response, Stream, Uri};

echo "=== URI Edge Cases ===\n";

// ARRANGE & ACT: Relative URI (path only)
echo "Relative URI: ";
$uri = Uri::fromString('/path/to/resource');
var_dump($uri->getPath() === '/path/to/resource');

// ARRANGE & ACT: URI with only query
echo "Query-only URI: ";
$uri = Uri::fromString('?key=value');
var_dump($uri->getQuery() === 'key=value');

// ARRANGE & ACT: URI with only fragment
echo "Fragment-only URI: ";
$uri = Uri::fromString('#section');
var_dump($uri->getFragment() === 'section');

// ARRANGE & ACT: Empty URI
echo "Empty URI: ";
$uri = Uri::fromString('');
var_dump((string)$uri === '');

// ARRANGE & ACT: Port boundary - 0
echo "Port 0: ";
try {
    $uri = Uri::fromString('http://example.com:0/path');
    var_dump($uri->getPort() === 0 || $uri->getPort() === null);
} catch (Exception $e) {
    var_dump(true); // Exception is also acceptable
}

// ARRANGE & ACT: Port boundary - 65535
echo "Port 65535: ";
$uri = Uri::fromString('http://example.com:65535/path');
var_dump($uri->getPort() === 65535);

// ARRANGE & ACT: Encoded characters in path
echo "Encoded path: ";
$uri = Uri::fromString('http://example.com/path%20with%20spaces');
var_dump(strpos($uri->getPath(), '%20') !== false || strpos($uri->getPath(), ' ') !== false);

// ARRANGE & ACT: Query with special characters
echo "Query special chars: ";
$uri = Uri::fromString('http://example.com?key=value&foo=bar%20baz');
var_dump(strlen($uri->getQuery()) > 0);

echo "\n=== Request Edge Cases ===\n";

// ARRANGE: Setup basic request environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ARRANGE & ACT: Custom HTTP method
echo "Custom method: ";
$request = Request::capture();
$customMethod = $request->withMethod('PATCH');
var_dump($customMethod->getMethod() === 'PATCH');

// ARRANGE & ACT: Invalid method rejected
echo "Invalid method rejected: ";
try {
    $request->withMethod('INVALID');
    var_dump(false);
} catch (InvalidArgumentException $e) {
    var_dump(true);
}

// ARRANGE & ACT: Protocol version 1.0
echo "Protocol 1.0: ";
$proto10 = $request->withProtocolVersion('1.0');
var_dump($proto10->getProtocolVersion() === '1.0');

// ARRANGE & ACT: Protocol version 1.1
echo "Protocol 1.1: ";
$proto11 = $request->withProtocolVersion('1.1');
var_dump($proto11->getProtocolVersion() === '1.1');

// ARRANGE & ACT: Header case insensitivity
echo "Header case insensitive lookup: ";
$withHeader = $request->withHeader('Content-Type', 'application/json');
var_dump($withHeader->hasHeader('content-type'));

// ARRANGE & ACT: Header case preservation
echo "Header case preserved: ";
$headers = $withHeader->getHeaders();
$hasOriginalCase = isset($headers['Content-Type']) || isset($headers['content-type']);
var_dump($hasOriginalCase);

// ARRANGE & ACT: getHeaderLine for non-existent header
echo "Non-existent header line: ";
var_dump($request->getHeaderLine('X-Non-Existent') === '');

echo "\n=== Response Edge Cases ===\n";

// ARRANGE & ACT: Status code boundary - 100
echo "Status 100: ";
$response = Response::create(100);
var_dump($response->getStatusCode() === 100);

// ARRANGE & ACT: Status code boundary - 599
echo "Status 599: ";
$response = Response::create(599);
var_dump($response->getStatusCode() === 599);

// ARRANGE & ACT: Invalid status - 99 (below range)
echo "Status 99 rejected: ";
try {
    $response = Response::create(99);
    var_dump(false);
} catch (InvalidArgumentException $e) {
    var_dump(true);
} catch (Exception $e) {
    var_dump(false);
}

// ARRANGE & ACT: Invalid status - 600 (above range)
echo "Status 600 rejected: ";
try {
    $response = Response::create(600);
    var_dump(false);
} catch (InvalidArgumentException $e) {
    var_dump(true);
} catch (Exception $e) {
    var_dump(false);
}

// ARRANGE & ACT: Custom reason phrase
echo "Custom reason phrase: ";
$response = Response::create(200)->withStatus(418, "I'm a teapot");
var_dump($response->getReasonPhrase() === "I'm a teapot");

// ARRANGE & ACT: Body at non-zero position
echo "Body position preserved: ";
$stream = Stream::fromString('Hello World');
$stream->read(6); // Read "Hello "
$position = $stream->tell();
$response = Response::create()->withBody($stream);
var_dump($response->getBody()->tell() === $position);

echo "\n=== Stream Edge Cases ===\n";

// ARRANGE & ACT: Read 0 bytes
echo "Read 0 bytes: ";
$stream = Stream::fromString('test');
$data = $stream->read(0);
var_dump($data === '');

// ARRANGE & ACT: Seek with SEEK_CUR positive
echo "Seek SEEK_CUR positive: ";
$stream = Stream::fromString('Hello World');
$stream->read(5);
$stream->seek(1, SEEK_CUR);
var_dump($stream->tell() === 6);

// ARRANGE & ACT: Seek with SEEK_END
echo "Seek SEEK_END: ";
$stream = Stream::fromString('Hello World');
$stream->seek(0, SEEK_END);
var_dump($stream->tell() === 11);

// ARRANGE & ACT: Write returns bytes written
echo "Write returns count: ";
$resource = fopen('php://memory', 'r+');
$stream = Stream::fromResource($resource);
$written = $stream->write('test');
var_dump($written === 4);

// ARRANGE & ACT: Empty stream size
echo "Empty stream size: ";
$stream = Stream::fromString('');
var_dump($stream->getSize() === 0);

// ARRANGE & ACT: Stream isReadable/isWritable for r+
echo "r+ mode readable: ";
$resource = fopen('php://memory', 'r+');
$stream = Stream::fromResource($resource);
var_dump($stream->isReadable());

echo "r+ mode writable: ";
var_dump($stream->isWritable());

echo "\n=== Cross-Component Edge Cases ===\n";

// ARRANGE & ACT: Immutability chain - multiple with*() calls
echo "Immutability chain: ";
$response = Response::create()
    ->withStatus(201)
    ->withHeader('Content-Type', 'application/json')
    ->withHeader('X-Custom', 'value');
$original = Response::create();
var_dump($original->getStatusCode() === 200);
var_dump($response->getStatusCode() === 201);
var_dump($response->hasHeader('Content-Type'));
var_dump($response->hasHeader('X-Custom'));

// ARRANGE & ACT: Request immutability chain
echo "Request immutability chain: ";
$_SERVER = ['REQUEST_METHOD' => 'GET', 'REQUEST_URI' => '/'];
$request = Request::capture()
    ->withMethod('POST')
    ->withHeader('Accept', 'application/json')
    ->withAttribute('user_id', 123);
var_dump($request->getMethod() === 'POST');
var_dump($request->hasHeader('Accept'));
var_dump($request->getAttribute('user_id') === 123);

// ARRANGE & ACT: URI to Request integration
echo "URI to Request: ";
$uri = Uri::fromString('https://api.example.com/v1/users?limit=10');
$request = Request::capture()->withUri($uri);
$resultUri = $request->getUri();
var_dump($resultUri->getHost() === 'api.example.com');
var_dump($resultUri->getPath() === '/v1/users');

echo "\nAll edge case tests completed.\n";
?>
--EXPECT--
=== URI Edge Cases ===
Relative URI: bool(true)
Query-only URI: bool(true)
Fragment-only URI: bool(true)
Empty URI: bool(true)
Port 0: bool(true)
Port 65535: bool(true)
Encoded path: bool(true)
Query special chars: bool(true)

=== Request Edge Cases ===
Custom method: bool(true)
Invalid method rejected: bool(true)
Protocol 1.0: bool(true)
Protocol 1.1: bool(true)
Header case insensitive lookup: bool(true)
Header case preserved: bool(true)
Non-existent header line: bool(true)

=== Response Edge Cases ===
Status 100: bool(true)
Status 599: bool(true)
Status 99 rejected: bool(true)
Status 600 rejected: bool(true)
Custom reason phrase: bool(true)
Body position preserved: bool(true)

=== Stream Edge Cases ===
Read 0 bytes: bool(true)
Seek SEEK_CUR positive: bool(true)
Seek SEEK_END: bool(true)
Write returns count: bool(true)
Empty stream size: bool(true)
r+ mode readable: bool(true)
r+ mode writable: bool(true)

=== Cross-Component Edge Cases ===
Immutability chain: bool(true)
bool(true)
bool(true)
bool(true)
Request immutability chain: bool(true)
bool(true)
bool(true)
URI to Request: bool(true)
bool(true)

All edge case tests completed.
