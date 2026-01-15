--TEST--
Client: Connection refused on closed port
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

// ARRANGE: Create request to closed port using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'http://localhost:9999');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    echo "Request should have been refused\n";
} catch (NetworkException $e) {
    // ASSERT: Verify connection refused exception was thrown
    var_dump(true);
    echo "Connection refused caught successfully\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Connection refused caught successfully
