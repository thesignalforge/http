--TEST--
Client: Invalid URL format
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
use Signalforge\NativeHttp\RequestException;

// ARRANGE: Create request with invalid URL using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();

try {
    $request = $requestFactory->createRequest('GET', 'not-a-valid-url');

    // ACT: Send the request
    try {
        $response = $client->sendRequest($request);
        echo "Request should have failed\n";
    } catch (RequestException $e) {
        // ASSERT: Verify request exception was thrown
        var_dump(true);
        echo "Invalid URL caught successfully\n";
    } catch (\Exception $e) {
        var_dump(true);
        echo "Invalid URL caught successfully\n";
    }
} catch (\Exception $e) {
    // Invalid URL caught during Request construction
    var_dump(true);
    echo "Invalid URL caught successfully\n";
}
?>
--EXPECT--
bool(true)
Invalid URL caught successfully
