--TEST--
Client: Follow multiple redirects (chain)
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

// ARRANGE: Create client that follows redirects using PSR-17 factory
$client = new Client(['follow_redirects' => true, 'max_redirects' => 10, 'timeout' => 15]);
$requestFactory = new RequestFactory();
// Use httpbin.org's redirect endpoint which chains N redirects
$request = $requestFactory->createRequest('GET', 'https://httpbin.org/redirect/3');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify redirects were followed (3 redirects then final /get)
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['url']));
    var_dump($data['url'] === 'https://httpbin.org/get');
    echo "Multiple redirects followed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Multiple redirects followed successfully
