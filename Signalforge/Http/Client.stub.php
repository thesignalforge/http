<?php
/**
 * Signalforge HTTP Extension
 * Client.stub.php - IDE stub for Client class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance PSR-18 HTTP Client implementation.
 *
 * Provides synchronous HTTP request execution using libcurl with:
 * - Connection pooling and reuse
 * - HTTP/1.1, HTTP/2, and HTTP/3 support
 * - Automatic retry with exponential backoff
 * - SSL/TLS verification
 * - Proxy support
 *
 * @final
 * @implements \Psr\Http\Client\ClientInterface
 */
final class Client implements \Psr\Http\Client\ClientInterface
{
    /**
     * HTTP version constants.
     */
    public const HTTP_VERSION_AUTO = 0;
    public const HTTP_VERSION_1_0 = 1;
    public const HTTP_VERSION_1_1 = 2;
    public const HTTP_VERSION_2_0 = 3;
    public const HTTP_VERSION_3_0 = 4;

    /**
     * Create a new HTTP client instance.
     *
     * @param array $options Configuration options:
     *   - pool_size: (int) Connection pool size (default: 8)
     *   - connect_timeout: (int) Connection timeout in seconds (default: 10)
     *   - timeout: (int) Request timeout in seconds (default: 30)
     *   - http_version: (string|int) HTTP version: "1.0", "1.1", "2", "3", or constant (default: auto)
     *   - max_redirects: (int) Maximum redirects to follow (default: 5)
     *   - follow_redirects: (bool) Whether to follow redirects (default: true)
     *   - verify_peer: (bool) Verify SSL peer certificate (default: true)
     *   - verify_host: (bool) Verify SSL host (default: true)
     *   - proxy: (string) Proxy URL (default: null)
     *   - user_agent: (string) User-Agent header (default: null)
     *   - ca_cert: (string) Path to CA certificate bundle (default: null)
     *   - retry: (array) Retry configuration:
     *       - max: (int) Maximum retry attempts (default: 0)
     *       - delay: (int) Initial delay in milliseconds (default: 1000)
     *       - max_delay: (int) Maximum delay in milliseconds (default: 60000)
     *       - backoff: (float) Backoff multiplier (default: 2.0)
     *   - use_threads: (bool) Use thread pool for parallel execution (ZTS only, default: false)
     *   - debug: (bool) Enable debug output (default: false)
     *
     * @throws HttpException If client initialization fails
     */
    public function __construct(array $options = []) {}

    /**
     * Sends a PSR-7 request and returns a PSR-7 response.
     *
     * This method is PSR-18 compliant and blocks until the response is received.
     *
     * @param \Psr\Http\Message\RequestInterface $request The request to send
     * @return \Psr\Http\Message\ResponseInterface The response
     * @throws NetworkException If a network error occurs (connection failed, timeout, etc.)
     * @throws RequestException If the request is invalid or cannot be sent
     */
    public function sendRequest(\Psr\Http\Message\RequestInterface $request): \Psr\Http\Message\ResponseInterface {}
}
