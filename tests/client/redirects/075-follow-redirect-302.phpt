--TEST--
Client: Follow 302 redirect automatically
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

// ARRANGE: Create client with redirect following enabled (default) using PSR-17 factory
$client = new Client(['follow_redirects' => true, 'max_redirects' => 5, 'timeout' => 10]);
$requestFactory = new RequestFactory();
// Use httpbin.org's redirect-to endpoint which redirects to specified URL
$request = $requestFactory->createRequest('GET', 'https://httpbin.org/redirect-to?url=https%3A%2F%2Fhttpbin.org%2Fget&status_code=302');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify redirect was followed and final destination reached
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['url']));
    var_dump($data['url'] === 'https://httpbin.org/get');
    echo "Redirect followed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Redirect followed successfully
