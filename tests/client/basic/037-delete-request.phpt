--TEST--
Client: DELETE request
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

// ARRANGE: Create DELETE request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('DELETE', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify DELETE was successful
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['id']));
    var_dump(isset($data['isDeleted']) && $data['isDeleted'] === true);
    echo "DELETE request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
DELETE request successful
