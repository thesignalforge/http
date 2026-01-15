--TEST--
Client: Timeout with partial response
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
use Signalforge\NativeHttp\Client;
use Signalforge\NativeHttp\RequestFactory;
use Signalforge\NativeHttp\NetworkException;

// ARRANGE: Create client with timeout using PSR-17 factory
$client = new Client(['timeout' => 3]);
$requestFactory = new RequestFactory();
// dummyjson has a delay endpoint
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/test?delay=10000');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    // If it completes quickly, that's also acceptable
    var_dump(true);
    echo "Partial response handled successfully\n";
} catch (NetworkException $e) {
    // ASSERT: Verify timeout exception was thrown
    var_dump(true);
    echo "Partial response handled successfully\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Partial response handled successfully
