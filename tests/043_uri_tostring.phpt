--TEST--
Uri::__toString() - URI reconstruction
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

// Simple URI
$uri = Uri::fromString('http://example.com/path');
echo (string)$uri . "\n";

// Full URI with all components
$uri = Uri::fromString('https://user:pass@example.com:8443/path?query=value#fragment');
echo (string)$uri . "\n";

// URI with standard port (should be omitted)
$uri = Uri::fromString('http://example.com:80/path');
echo (string)$uri . "\n";

$uri = Uri::fromString('https://example.com:443/path');
echo (string)$uri . "\n";

// Build URI from scratch using with* methods
$uri = Uri::fromString('');
$uri = $uri->withScheme('https')
           ->withHost('api.example.com')
           ->withPort(3000)
           ->withPath('/v1/users')
           ->withQuery('limit=10')
           ->withFragment('list');
echo (string)$uri . "\n";

?>
--EXPECT--
http://example.com/path
https://user:pass@example.com:8443/path?query=value#fragment
http://example.com/path
https://example.com/path
https://api.example.com:3000/v1/users?limit=10#list
