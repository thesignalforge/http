<?php
/**
 * Signalforge HTTP Extension
 * HttpRequestPool.stub.php - IDE stub for HttpRequestPool class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * Concurrent HTTP request pool for executing multiple requests in parallel.
 *
 * Uses curl_multi internally for efficient parallel execution without threads,
 * or optionally uses a thread pool in ZTS builds for true parallelism.
 *
 * Example usage:
 * ```php
 * $client = new Client();
 * $pool = new HttpRequestPool($client, concurrency: 10);
 *
 * $pool->add($request1, onSuccess: fn($response) => handleSuccess($response));
 * $pool->add($request2, onError: fn($response) => handleError($response));
 *
 * $responses = $pool->wait(); // Execute all and get responses
 * ```
 *
 * @final
 */
final class HttpRequestPool
{
    /**
     * Create a new request pool.
     *
     * @param Client $client The HTTP client to use for requests
     * @param int $concurrency Maximum number of concurrent requests (default: 50)
     * @throws HttpException If pool initialization fails
     */
    public function __construct(Client $client, int $concurrency = 50) {}

    /**
     * Add a request to the pool.
     *
     * @param \Psr\Http\Message\RequestInterface $request The request to add
     * @param callable|null $onSuccess Callback invoked on successful response: fn(ResponseInterface $response)
     * @param callable|null $onError Callback invoked on error: fn(ResponseInterface $response)
     * @return void
     * @throws HttpException If the pool has been cancelled
     */
    public function add(
        \Psr\Http\Message\RequestInterface $request,
        ?callable $onSuccess = null,
        ?callable $onError = null
    ): void {}

    /**
     * Execute all pending requests and wait for completion.
     *
     * Callbacks (onSuccess/onError) are invoked as responses arrive.
     * Returns an array of all responses indexed by request order.
     *
     * @return array<int, \Psr\Http\Message\ResponseInterface> Array of responses
     * @throws HttpException If execution fails
     */
    public function wait(): array {}

    /**
     * Cancel all pending requests.
     *
     * After calling cancel(), no more requests can be added.
     *
     * @return void
     */
    public function cancel(): void {}
}
