--TEST--
signalforge_http: Extension loads and classes exist
--EXTENSIONS--
signalforge_http
--FILE--
<?php
// ARRANGE: Extension should be loaded by --EXTENSIONS-- directive

// ACT & ASSERT: Verify extension is loaded
var_dump(extension_loaded('signalforge_http'));

// ACT & ASSERT: Verify core classes exist
var_dump(class_exists('Signalforge\NativeHttp\Request'));
var_dump(class_exists('Signalforge\NativeHttp\Response'));
var_dump(class_exists('Signalforge\NativeHttp\Stream'));

// ACT & ASSERT: Verify classes are final (cannot be extended)
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

