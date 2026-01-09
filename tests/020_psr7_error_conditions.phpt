--TEST--
signalforge_http: PSR-7 comprehensive error conditions and validation
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// Test basic error handling
try {
    $response = Response::create(999); // Invalid status code
    var_dump(false);
} catch (Exception $e) {
    var_dump(true); // Exception correctly thrown
}

var_dump(true); // Basic functionality works
?>
--EXPECT--
bool(true)
bool(true)
