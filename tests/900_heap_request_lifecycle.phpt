--TEST--
Request object lifecycle - heap corruption prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// ARRANGE: Test object creation and destruction patterns that previously caused heap corruption

echo "=== Test 1: Multiple Request::create() with Uri objects ===\n";
// This specifically targets the double-free bug in Request::create()
for ($i = 0; $i < 20; $i++) {
    $uri = Uri::fromString("http://test{$i}.com/path");
    $request = Request::create('GET', $uri);
    $retrieved = $request->getUri();

    // ACT: Access URI properties to ensure object is valid
    $host = $retrieved->getHost();

    // ASSERT: Uri object should still be valid after retrieval
    var_dump($host === "test{$i}.com");
}
echo "All iterations passed\n";

echo "\n=== Test 2: Request with method chains (immutability stress test) ===\n";
// Each with*() creates a new object - tests proper cleanup of intermediate objects
$base = Request::create('GET', 'http://example.com');
$r1 = $base->withMethod('POST');
$r2 = $r1->withHeader('X-Custom', 'value');
$r3 = $r2->withAddedHeader('X-Custom', 'value2');
$r4 = $r3->withoutHeader('X-Custom');
$r5 = $r4->withBody($base->getBody());

// ASSERT: Final object should be valid
var_dump($r5->getMethod() === 'POST');
var_dump(count($r5->getHeaders()) >= 0); // Should not crash
echo "Immutability chain passed\n";

echo "\n=== Test 3: Reusing Uri object across multiple Requests ===\n";
// Tests that Uri refcount is properly managed when shared
$shared_uri = Uri::fromString('http://shared.example.com/resource');
$requests = [];
for ($i = 0; $i < 10; $i++) {
    $requests[] = Request::create('GET', $shared_uri);
}

// ACT: Access all requests to ensure Uri is still valid
foreach ($requests as $idx => $req) {
    $uri = $req->getUri();
    // ASSERT: Uri should be accessible and correct
    var_dump($uri->getHost() === 'shared.example.com');
}
echo "Shared Uri passed\n";

echo "\n=== Test 4: String URI vs Uri object ownership ===\n";
// Tests different ownership paths in Request::create()
$string_req = Request::create('POST', 'http://string-uri.com/test');
$object_req = Request::create('POST', Uri::fromString('http://object-uri.com/test'));

// ASSERT: Both should work correctly
var_dump($string_req->getUri()->getHost() === 'string-uri.com');
var_dump($object_req->getUri()->getHost() === 'object-uri.com');
echo "URI ownership passed\n";

echo "\n=== Test 5: Rapid object creation/destruction ===\n";
// Stress test for memory allocation/deallocation patterns
for ($i = 0; $i < 100; $i++) {
    $req = Request::create('PUT', "http://rapid{$i}.com/test");
    $method = $req->getMethod();
    $uri = $req->getUri();
    // Object destroyed at end of loop - tests free_obj handler
}
echo "Rapid lifecycle passed\n";

echo "\nAll heap corruption tests passed\n";
?>
--EXPECT--
=== Test 1: Multiple Request::create() with Uri objects ===
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
All iterations passed

=== Test 2: Request with method chains (immutability stress test) ===
bool(true)
bool(true)
Immutability chain passed

=== Test 3: Reusing Uri object across multiple Requests ===
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
Shared Uri passed

=== Test 4: String URI vs Uri object ownership ===
bool(true)
bool(true)
URI ownership passed

=== Test 5: Rapid object creation/destruction ===
Rapid lifecycle passed

All heap corruption tests passed
