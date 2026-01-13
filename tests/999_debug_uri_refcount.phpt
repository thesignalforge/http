--TEST--
Debug: Uri object refcount in Request::create() - PHP 8.5 heap corruption investigation
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// ARRANGE: Test Uri object lifecycle in Request::create()
// This test specifically targets the heap corruption seen in CI at test ~63/210

echo "=== Test 1: Basic Uri object in Request::create() ===\n";
$uri = Uri::fromString('http://example.com/path');
$request = Request::create('GET', $uri);
var_dump($request->getUri() instanceof Uri);
echo "URI: " . $request->getUri() . "\n";

echo "\n=== Test 2: Multiple Uri conversions (stress test) ===\n";
for ($i = 0; $i < 10; $i++) {
    $uri = Uri::fromString("http://test{$i}.com/path{$i}");
    $req = Request::create('POST', $uri);
    $retrieved_uri = $req->getUri();
    echo "Iteration {$i}: " . $retrieved_uri . "\n";
    // Explicitly test that Uri object is still valid
    var_dump($retrieved_uri->getHost() === "test{$i}.com");
}

echo "\n=== Test 3: Uri with complex components ===\n";
$uri = Uri::fromString('https://user:pass@example.com:8080/path?query=value#fragment');
$request = Request::create('PUT', $uri);
$retrieved = $request->getUri();
echo "Scheme: " . $retrieved->getScheme() . "\n";
echo "Host: " . $retrieved->getHost() . "\n";
echo "Port: " . ($retrieved->getPort() ?? 'null') . "\n";
echo "Path: " . $retrieved->getPath() . "\n";
echo "Query: " . $retrieved->getQuery() . "\n";
echo "Fragment: " . $retrieved->getFragment() . "\n";

echo "\n=== Test 4: Reusing same Uri object multiple times ===\n";
$uri = Uri::fromString('http://shared.com/resource');
for ($i = 0; $i < 5; $i++) {
    $req = Request::create('GET', $uri);
    echo "Shared iteration {$i}: " . $req->getUri() . "\n";
}

echo "\n=== Test 5: String URI vs Uri object ===\n";
$req1 = Request::create('GET', 'http://string-uri.com/path');
$req2 = Request::create('GET', Uri::fromString('http://object-uri.com/path'));
echo "String URI: " . $req1->getUri() . "\n";
echo "Object URI: " . $req2->getUri() . "\n";

echo "\nAll tests completed successfully\n";
?>
--EXPECT--
=== Test 1: Basic Uri object in Request::create() ===
bool(true)
URI: http://example.com/path

=== Test 2: Multiple Uri conversions (stress test) ===
Iteration 0: http://test0.com/path0
bool(true)
Iteration 1: http://test1.com/path1
bool(true)
Iteration 2: http://test2.com/path2
bool(true)
Iteration 3: http://test3.com/path3
bool(true)
Iteration 4: http://test4.com/path4
bool(true)
Iteration 5: http://test5.com/path5
bool(true)
Iteration 6: http://test6.com/path6
bool(true)
Iteration 7: http://test7.com/path7
bool(true)
Iteration 8: http://test8.com/path8
bool(true)
Iteration 9: http://test9.com/path9
bool(true)

=== Test 3: Uri with complex components ===
Scheme: https
Host: example.com
Port: 8080
Path: /path
Query: query=value
Fragment: fragment

=== Test 4: Reusing same Uri object multiple times ===
Shared iteration 0: http://shared.com/resource
Shared iteration 1: http://shared.com/resource
Shared iteration 2: http://shared.com/resource
Shared iteration 3: http://shared.com/resource
Shared iteration 4: http://shared.com/resource

=== Test 5: String URI vs Uri object ===
String URI: http://string-uri.com/path
Object URI: http://object-uri.com/path

All tests completed successfully
