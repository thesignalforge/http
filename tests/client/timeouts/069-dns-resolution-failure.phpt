--TEST--
Client: DNS resolution failure for invalid hostname
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

// ARRANGE: Create request to non-existent domain using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://this-domain-does-not-exist-12345.invalid');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    echo "Request should have failed DNS resolution\n";
} catch (NetworkException $e) {
    // ASSERT: Verify DNS failure exception was thrown
    var_dump(true);
    echo "DNS resolution failure caught successfully\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
DNS resolution failure caught successfully
