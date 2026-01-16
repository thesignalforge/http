--TEST--
Client: RequestException class exists and extends HttpException
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
use Signalforge\NativeHttp\RequestException;

// Arrange: (no setup needed)

// Act: Check class hierarchy
$exists = class_exists(RequestException::class);
$extends = is_subclass_of(RequestException::class, HttpException::class);

// Assert: RequestException should exist and extend HttpException
var_dump($exists);
var_dump($extends);
?>
--EXPECT--
bool(true)
bool(true)
