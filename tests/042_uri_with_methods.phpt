--TEST--
Uri with*() methods - immutable modifiers
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Uri;

$uri = Uri::fromString('http://example.com/path');

// Test withScheme
$new = $uri->withScheme('https');
var_dump($uri->getScheme()); // Original unchanged
var_dump($new->getScheme()); // New has updated value

echo "---\n";

// Test withHost
$new = $uri->withHost('other.com');
var_dump($uri->getHost());
var_dump($new->getHost());

echo "---\n";

// Test withPort
$new = $uri->withPort(8080);
var_dump($uri->getPort());
var_dump($new->getPort());

// Test withPort(null) to remove port
$new2 = $new->withPort(null);
var_dump($new2->getPort());

echo "---\n";

// Test withPath
$new = $uri->withPath('/new/path');
var_dump($uri->getPath());
var_dump($new->getPath());

echo "---\n";

// Test withQuery
$new = $uri->withQuery('foo=bar');
var_dump($uri->getQuery());
var_dump($new->getQuery());

echo "---\n";

// Test withFragment
$new = $uri->withFragment('top');
var_dump($uri->getFragment());
var_dump($new->getFragment());

echo "---\n";

// Test withUserInfo
$new = $uri->withUserInfo('admin', 'secret');
var_dump($uri->getUserInfo());
var_dump($new->getUserInfo());

?>
--EXPECT--
string(4) "http"
string(5) "https"
---
string(11) "example.com"
string(9) "other.com"
---
NULL
int(8080)
NULL
---
string(5) "/path"
string(9) "/new/path"
---
string(0) ""
string(7) "foo=bar"
---
string(0) ""
string(3) "top"
---
string(0) ""
string(12) "admin:secret"
