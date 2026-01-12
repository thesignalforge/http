--TEST--
Uri object zval ownership - refcount and memory leak prevention
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// ARRANGE: Test Uri object lifetime and zval ownership patterns

echo "=== Test 1: Uri object passed to Request::create() ===\n";
// This was the primary heap corruption bug - Uri::__toString() refcount mismanagement
for ($i = 0; $i < 30; $i++) {
    $uri = Uri::fromString("http://host{$i}.com/path{$i}");
    $req = Request::create('GET', $uri);

    // ACT: Retrieve Uri and access its properties
    $retrieved_uri = $req->getUri();
    $host = $retrieved_uri->getHost();
    $path = $retrieved_uri->getPath();

    // ASSERT: Uri should be valid and correct
    var_dump($host === "host{$i}.com");
    var_dump($path === "/path{$i}");
}
echo "Uri ownership passed\n";

echo "\n=== Test 2: withUri() immutability and object retention ===\n";
$base_req = Request::create('GET', 'http://original.com/path');
$new_uri = Uri::fromString('http://updated.com/newpath');
$updated_req = $base_req->withUri($new_uri);

// ASSERT: Both requests should have valid URIs
var_dump($base_req->getUri()->getHost() === 'original.com');
var_dump($updated_req->getUri()->getHost() === 'updated.com');

// ACT: Further modifications
$another_uri = Uri::fromString('http://third.com/anotherpath');
$third_req = $updated_req->withUri($another_uri);

// ASSERT: All three should still be valid
var_dump($base_req->getUri()->getHost() === 'original.com');
var_dump($updated_req->getUri()->getHost() === 'updated.com');
var_dump($third_req->getUri()->getHost() === 'third.com');
echo "withUri immutability passed\n";

echo "\n=== Test 3: Uri with complex components lifecycle ===\n";
// Test that all Uri components are properly managed
for ($i = 0; $i < 15; $i++) {
    $uri = Uri::fromString("https://user{$i}:pass{$i}@host{$i}.com:808{$i}/path{$i}?query{$i}=val{$i}#frag{$i}");
    $req = Request::create('POST', $uri);
    $u = $req->getUri();

    // ASSERT: All components should be correct
    var_dump($u->getUserInfo() !== '');
    var_dump($u->getHost() === "host{$i}.com");
    var_dump($u->getPath() === "/path{$i}");
    var_dump(strpos($u->getQuery(), "query{$i}=val{$i}") !== false);
}
echo "Complex Uri lifecycle passed\n";

echo "\n=== Test 4: Multiple requests sharing same Uri object ===\n";
// Tests proper refcount management when Uri is shared
$shared_uri = Uri::fromString('http://shared-resource.com/api/endpoint');
$requests = [];

for ($i = 0; $i < 20; $i++) {
    $requests[] = Request::create('GET', $shared_uri);
}

// ASSERT: All requests should have the same Uri
foreach ($requests as $idx => $req) {
    $uri = $req->getUri();
    var_dump($uri->getHost() === 'shared-resource.com');
}
echo "Shared Uri refcount passed\n";

echo "\n=== Test 5: Uri::fromString() with various formats ===\n";
$formats = [
    'http://example.com',
    'https://example.com:443',
    'http://example.com/path',
    'http://example.com/path?query=value',
    'http://example.com/path?query=value#fragment',
    'http://user@example.com/path',
    'http://user:pass@example.com:8080/path?q=v#f',
];

foreach ($formats as $format) {
    $uri = Uri::fromString($format);
    $req = Request::create('GET', $uri);
    $retrieved = $req->getUri();

    // ASSERT: Uri should be valid
    var_dump($retrieved instanceof Uri);
    var_dump(strlen((string)$retrieved) > 0);
}
echo "Various formats passed\n";

echo "\n=== Test 6: withUri preserveHost parameter ===\n";
$req = Request::create('GET', 'http://original-host.com/path');
$req = $req->withHeader('Host', 'custom-host.com');

$new_uri = Uri::fromString('http://new-host.com/newpath');
$req_preserve = $req->withUri($new_uri, true); // preserveHost = true

// ASSERT: Host header should be preserved
var_dump($req_preserve->getHeaderLine('Host') === 'custom-host.com');
echo "preserveHost passed\n";

echo "\nAll Uri zval ownership tests passed\n";
?>
--EXPECT--
=== Test 1: Uri object passed to Request::create() ===
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
Uri ownership passed

=== Test 2: withUri() immutability and object retention ===
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
withUri immutability passed

=== Test 3: Uri with complex components lifecycle ===
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
Complex Uri lifecycle passed

=== Test 4: Multiple requests sharing same Uri object ===
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
Shared Uri refcount passed

=== Test 5: Uri::fromString() with various formats ===
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
Various formats passed

=== Test 6: withUri preserveHost parameter ===
bool(true)
preserveHost passed

All Uri zval ownership tests passed
