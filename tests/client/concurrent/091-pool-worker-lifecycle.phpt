--TEST--
Client: Thread pool worker lifecycle with varying pool sizes
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
/**
 * Tests thread pool worker lifecycle with different pool size configurations.
 * Verifies that workers are properly created, used, and cleaned up.
 */
use Signalforge\NativeHttp\Client;
use Signalforge\NativeHttp\HttpRequestPool;
use Signalforge\NativeHttp\RequestFactory;

$requestFactory = new RequestFactory();

echo "=== Test 1: Single worker (pool_size=1) ===\n";
$client = new Client(['pool_size' => 1, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 5);

for ($i = 1; $i <= 3; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool->add($request);
}

$responses = $pool->wait();
var_dump(count($responses) === 3);
echo "Single worker completed\n";

echo "\n=== Test 2: Large pool (pool_size=8) ===\n";
$client2 = new Client(['pool_size' => 8, 'timeout' => 30]);
$pool2 = new HttpRequestPool($client2, 10);

for ($i = 1; $i <= 6; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool2->add($request);
}

$responses = $pool2->wait();
var_dump(count($responses) === 6);
echo "Large pool completed\n";

echo "\n=== Test 3: Requests exceed pool capacity ===\n";
$client3 = new Client(['pool_size' => 2, 'timeout' => 30]);
$pool3 = new HttpRequestPool($client3, 20);

// Add more requests than pool_size to test queuing
for ($i = 1; $i <= 8; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool3->add($request);
}

$responses = $pool3->wait();
var_dump(count($responses) === 8);
echo "Queue overflow handled correctly\n";

echo "\n=== Test 4: Multiple pool instances ===\n";
$clientA = new Client(['pool_size' => 2, 'timeout' => 30]);
$clientB = new Client(['pool_size' => 2, 'timeout' => 30]);
$poolA = new HttpRequestPool($clientA, 5);
$poolB = new HttpRequestPool($clientB, 5);

for ($i = 1; $i <= 3; $i++) {
    $reqA = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $reqB = $requestFactory->createRequest('GET', 'https://dummyjson.com/users/' . $i);
    $poolA->add($reqA);
    $poolB->add($reqB);
}

$responsesA = $poolA->wait();
$responsesB = $poolB->wait();
var_dump(count($responsesA) === 3);
var_dump(count($responsesB) === 3);
echo "Multiple pools completed independently\n";

echo "\nAll worker lifecycle tests passed!\n";
?>
--EXPECT--
=== Test 1: Single worker (pool_size=1) ===
bool(true)
Single worker completed

=== Test 2: Large pool (pool_size=8) ===
bool(true)
Large pool completed

=== Test 3: Requests exceed pool capacity ===
bool(true)
Queue overflow handled correctly

=== Test 4: Multiple pool instances ===
bool(true)
bool(true)
Multiple pools completed independently

All worker lifecycle tests passed!
