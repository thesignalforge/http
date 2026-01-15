--TEST--
Client: Request timeout on slow response
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

// ARRANGE: Create client with short timeout using PSR-17 factory
$client = new Client(['timeout' => 2]);
$requestFactory = new RequestFactory();
// dummyjson has a delay endpoint
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/test?delay=5000');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    echo "Request should have timed out\n";
} catch (NetworkException $e) {
    // ASSERT: Verify timeout exception was thrown
    var_dump(true);
    echo "Request timeout caught successfully\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Request timeout caught successfully
