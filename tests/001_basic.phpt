--TEST--
signalforge_http: Extension loads and classes exist
--EXTENSIONS--
signalforge_http
--FILE--
<?php
var_dump(extension_loaded('signalforge_http'));

var_dump(class_exists('Signalforge\NativeHttp\Request'));
var_dump(class_exists('Signalforge\NativeHttp\Response'));
var_dump(class_exists('Signalforge\NativeHttp\Stream'));

// Check that classes are final
$reflection = new ReflectionClass('Signalforge\NativeHttp\Request');
var_dump($reflection->isFinal());

$reflection = new ReflectionClass('Signalforge\NativeHttp\Response');
var_dump($reflection->isFinal());

$reflection = new ReflectionClass('Signalforge\NativeHttp\Stream');
var_dump($reflection->isFinal());
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

