--TEST--
Client: Connection timeout on unreachable host
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

// ARRANGE: Create client with short connection timeout using PSR-17 factory
$client = new Client(['connect_timeout' => 1, 'timeout' => 5]);
$requestFactory = new RequestFactory();
// Use non-routable IP address that will timeout
$request = $requestFactory->createRequest('GET', 'http://10.255.255.1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    echo "Request should have timed out\n";
} catch (NetworkException $e) {
    // ASSERT: Verify timeout exception was thrown
    var_dump(true);
    echo "Connection timeout caught successfully\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Connection timeout caught successfully
