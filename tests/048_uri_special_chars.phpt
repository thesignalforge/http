--TEST--
signalforge_http: Uri handles special characters in components
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Test URL-encoded characters in path
$uri = Uri::fromString('http://example.com/path%20with%20spaces');
echo "Encoded path: ";
var_dump($uri->getPath());

// Test special chars in query
$uri = Uri::fromString('http://example.com/search?q=hello+world&filter=a%26b');
echo "Query with special chars: ";
var_dump($uri->getQuery());

// Test multiple query parameters
$uri = Uri::fromString('http://example.com/api?name=John&age=30&city=New%20York');
echo "Multiple params: ";
var_dump($uri->getQuery());

// Test userinfo with special chars
$uri = Uri::fromString('http://user%40domain:p%40ss@example.com/');
echo "Encoded userinfo: ";
var_dump($uri->getUserInfo());

// Test fragment with special chars
$uri = Uri::fromString('http://example.com/doc#section%201');
echo "Encoded fragment: ";
var_dump($uri->getFragment());

// Test unicode in path (should be percent-encoded)
$uri = Uri::fromString('http://example.com/caf%C3%A9');
echo "Unicode path: ";
var_dump($uri->getPath());

echo "Special character tests passed\n";
?>
--EXPECT--
Encoded path: string(21) "/path%20with%20spaces"
Query with special chars: string(26) "q=hello+world&filter=a%26b"
Multiple params: string(32) "name=John&age=30&city=New%20York"
Encoded userinfo: string(20) "user%40domain:p%40ss"
Encoded fragment: string(11) "section%201"
Unicode path: string(10) "/caf%C3%A9"
Special character tests passed
