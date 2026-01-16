--TEST--
Client: Don't follow redirects when disabled
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

// ARRANGE: Create client with redirect following disabled using PSR-17 factory
$client = new Client(['follow_redirects' => false, 'timeout' => 10]);
$requestFactory = new RequestFactory();
// Use httpbin.org's redirect-to endpoint
$request = $requestFactory->createRequest('GET', 'https://httpbin.org/redirect-to?url=https%3A%2F%2Fhttpbin.org%2Fget&status_code=302');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();
    $hasLocation = $response->hasHeader('Location');

    // ASSERT: Verify redirect status is returned without following
    var_dump(in_array($statusCode, [301, 302, 303, 307, 308]));
    var_dump($hasLocation);
    echo "Redirect not followed as expected\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
Redirect not followed as expected
