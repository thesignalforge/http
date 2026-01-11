--TEST--
Uri - IPv6 host handling
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// IPv6 localhost
$uri = Uri::fromString('http://[::1]/path');
var_dump($uri->getHost());
var_dump($uri->getPath());
echo (string)$uri . "\n";

// IPv6 with port
$uri = Uri::fromString('http://[::1]:8080/path');
var_dump($uri->getHost());
var_dump($uri->getPort());
echo (string)$uri . "\n";

// Full IPv6 address
$uri = Uri::fromString('https://[2001:db8::1]/resource');
var_dump($uri->getHost());
echo (string)$uri . "\n";

?>
--EXPECT--
string(5) "[::1]"
string(5) "/path"
http://[::1]/path
string(5) "[::1]"
int(8080)
http://[::1]:8080/path
string(13) "[2001:db8::1]"
https://[2001:db8::1]/resource
