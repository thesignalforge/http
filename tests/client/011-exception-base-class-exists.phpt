--TEST--
Client: HttpException base class exists
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
// Arrange: (no setup needed)

// Act: Check if HttpException class exists
$exists = class_exists('Signalforge\\NativeHttp\\HttpException');

// Assert: HttpException class should exist and extend Exception
var_dump($exists);
if ($exists) {
    $reflection = new ReflectionClass('Signalforge\\NativeHttp\\HttpException');
    var_dump($reflection->isSubclassOf('Exception'));
}
?>
--EXPECT--
bool(true)
bool(true)
