--TEST--
HashTable ownership and header manipulation - memory leak prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Response;

// ARRANGE: Test HashTable header management and ownership

echo "=== Test 1: Request header manipulation chain ===\n";
// Each withHeader() clones the headers HashTable - tests proper cleanup
$req = Request::create('GET', 'http://test.com');
$req = $req->withHeader('Content-Type', 'application/json');
$req = $req->withHeader('Accept', 'application/json');
$req = $req->withHeader('X-Custom-1', 'value1');
$req = $req->withHeader('X-Custom-2', 'value2');
$req = $req->withHeader('X-Custom-3', 'value3');

// Modify existing headers
$req = $req->withHeader('Content-Type', 'text/html');
$req = $req->withoutHeader('X-Custom-2');
$req = $req->withAddedHeader('Accept', 'text/html');

// ASSERT: Headers should be correct
var_dump($req->getHeaderLine('Content-Type') === 'text/html');
var_dump($req->hasHeader('X-Custom-2') === false);
var_dump(count($req->getHeader('Accept')) === 2);
echo "Request header chain passed\n";

echo "\n=== Test 2: Response header manipulation chain ===\n";
$resp = new Response();
$resp = $resp->withHeader('Cache-Control', 'no-cache');
$resp = $resp->withHeader('Content-Type', 'application/json');
$resp = $resp->withAddedHeader('Cache-Control', 'no-store');

// Replace and remove
$resp = $resp->withHeader('Cache-Control', 'max-age=3600');
$resp = $resp->withoutHeader('Cache-Control');

// ASSERT: Headers should reflect changes
var_dump($resp->hasHeader('Cache-Control') === false);
var_dump($resp->hasHeader('Content-Type') === true);
echo "Response header chain passed\n";

echo "\n=== Test 3: Many headers stress test ===\n";
$req = Request::create('POST', 'http://example.com');
// Create 50 headers
for ($i = 0; $i < 50; $i++) {
    $req = $req->withHeader("X-Header-{$i}", "value{$i}");
}

// Remove every other header
for ($i = 0; $i < 50; $i += 2) {
    $req = $req->withoutHeader("X-Header-{$i}");
}

// ASSERT: Should have 25 headers remaining (plus any default headers)
$all_headers = $req->getHeaders();
$custom_headers = array_filter(array_keys($all_headers), function($name) {
    return strpos($name, 'x-header-') === 0;
});
var_dump(count($custom_headers) === 25);
echo "Many headers passed\n";

echo "\n=== Test 4: Header case-insensitivity test ===\n";
$req = Request::create('GET', 'http://test.com');
$req = $req->withHeader('Content-Type', 'application/json');

// Access with different cases
$exists1 = $req->hasHeader('content-type');
$exists2 = $req->hasHeader('CONTENT-TYPE');
$exists3 = $req->hasHeader('CoNtEnT-TyPe');

// ASSERT: All should work due to case-insensitive hash
var_dump($exists1 === true);
var_dump($exists2 === true);
var_dump($exists3 === true);
echo "Case insensitivity passed\n";

echo "\n=== Test 5: withAddedHeader array values ===\n";
$resp = new Response();
$resp = $resp->withHeader('Set-Cookie', 'cookie1=value1');
$resp = $resp->withAddedHeader('Set-Cookie', 'cookie2=value2');
$resp = $resp->withAddedHeader('Set-Cookie', 'cookie3=value3');

$cookies = $resp->getHeader('Set-Cookie');
// ASSERT: Should have 3 cookie values
var_dump(count($cookies) === 3);
var_dump(in_array('cookie1=value1', $cookies));
var_dump(in_array('cookie2=value2', $cookies));
var_dump(in_array('cookie3=value3', $cookies));
echo "Added header arrays passed\n";

echo "\n=== Test 6: Rapid header add/remove cycle ===\n";
$req = Request::create('DELETE', 'http://api.example.com');
for ($i = 0; $i < 100; $i++) {
    $req = $req->withHeader('X-Cycle', "iteration{$i}");
    $value = $req->getHeaderLine('X-Cycle');
    $req = $req->withoutHeader('X-Cycle');
}
// ASSERT: Header should be gone
var_dump($req->hasHeader('X-Cycle') === false);
echo "Rapid cycle passed\n";

echo "\nAll HashTable ownership tests passed\n";
?>
--EXPECT--
=== Test 1: Request header manipulation chain ===
bool(true)
bool(true)
bool(true)
Request header chain passed

=== Test 2: Response header manipulation chain ===
bool(true)
bool(true)
Response header chain passed

=== Test 3: Many headers stress test ===
bool(true)
Many headers passed

=== Test 4: Header case-insensitivity test ===
bool(true)
bool(true)
bool(true)
Case insensitivity passed

=== Test 5: withAddedHeader array values ===
bool(true)
bool(true)
bool(true)
bool(true)
Added header arrays passed

=== Test 6: Rapid header add/remove cycle ===
bool(true)
Rapid cycle passed

All HashTable ownership tests passed
