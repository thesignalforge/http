--TEST--
Request::getUri() returns Uri object
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;
use Signalforge\NativeHttp\Uri;

// Simulate server environment
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/users/123?include=profile',
    'HTTP_HOST' => 'api.example.com',
    'HTTPS' => 'on',
];

$request = Request::capture();
$uri = $request->getUri();

// Verify it's a Uri object
var_dump($uri instanceof Uri);

// Check components
var_dump($uri->getScheme());
var_dump($uri->getHost());
var_dump($uri->getPath());
var_dump($uri->getQuery());

// Check string representation
echo (string)$uri . "\n";

?>
--EXPECT--
bool(true)
string(5) "https"
string(15) "api.example.com"
string(10) "/users/123"
string(15) "include=profile"
https://api.example.com/users/123?include=profile
