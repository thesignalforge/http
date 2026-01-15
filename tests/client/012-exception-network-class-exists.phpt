--TEST--
Client: NetworkException class exists and extends HttpException
--EXTENSIONS--
signalforge_http
--SKIPIF--
<?php
if (!class_exists('Signalforge\NativeHttp\Client')) {
    die('skip PSR-18 client not available (requires libcurl)');
}
?>
--FILE--
<?php
use Signalforge\NativeHttp\HttpException;
use Signalforge\NativeHttp\NetworkException;

// Arrange: (no setup needed)

// Act: Check class hierarchy
$exists = class_exists(NetworkException::class);
$extends = is_subclass_of(NetworkException::class, HttpException::class);

// Assert: NetworkException should exist and extend HttpException
var_dump($exists);
var_dump($extends);
?>
--EXPECT--
bool(true)
bool(true)
