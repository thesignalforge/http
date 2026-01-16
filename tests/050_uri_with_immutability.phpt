--TEST--
signalforge_http: Uri with* methods return new instances (immutability)
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

$original = Uri::fromString('http://example.com/path');

// Test withScheme returns new instance
$modified = $original->withScheme('https');
echo "Original scheme unchanged: ";
var_dump($original->getScheme() === 'http');
echo "Modified scheme: ";
var_dump($modified->getScheme() === 'https');

// Test withHost returns new instance
$modified = $original->withHost('other.com');
echo "Original host unchanged: ";
var_dump($original->getHost() === 'example.com');
echo "Modified host: ";
var_dump($modified->getHost() === 'other.com');

// Test withPort returns new instance
$modified = $original->withPort(8080);
echo "Original port unchanged: ";
var_dump($original->getPort() === null);
echo "Modified port: ";
var_dump($modified->getPort() === 8080);

// Test withPath returns new instance
$modified = $original->withPath('/new/path');
echo "Original path unchanged: ";
var_dump($original->getPath() === '/path');
echo "Modified path: ";
var_dump($modified->getPath() === '/new/path');

// Test withQuery returns new instance
$modified = $original->withQuery('key=value');
echo "Original query unchanged: ";
var_dump($original->getQuery() === '');
echo "Modified query: ";
var_dump($modified->getQuery() === 'key=value');

// Test withFragment returns new instance
$modified = $original->withFragment('section');
echo "Original fragment unchanged: ";
var_dump($original->getFragment() === '');
echo "Modified fragment: ";
var_dump($modified->getFragment() === 'section');

// Test withUserInfo returns new instance
$modified = $original->withUserInfo('user', 'pass');
echo "Original userInfo unchanged: ";
var_dump($original->getUserInfo() === '');
echo "Modified userInfo: ";
var_dump($modified->getUserInfo() === 'user:pass');

echo "Immutability tests passed\n";
?>
--EXPECT--
Original scheme unchanged: bool(true)
Modified scheme: bool(true)
Original host unchanged: bool(true)
Modified host: bool(true)
Original port unchanged: bool(true)
Modified port: bool(true)
Original path unchanged: bool(true)
Modified path: bool(true)
Original query unchanged: bool(true)
Modified query: bool(true)
Original fragment unchanged: bool(true)
Modified fragment: bool(true)
Original userInfo unchanged: bool(true)
Modified userInfo: bool(true)
Immutability tests passed
