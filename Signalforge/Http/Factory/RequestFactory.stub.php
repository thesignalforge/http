<?php
/**
 * Signalforge HTTP Extension
 * RequestFactory.stub.php - IDE stub for RequestFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant request factory.
 *
 * Creates RequestInterface instances for outgoing HTTP requests.
 *
 * @final
 * @implements \Psr\Http\Message\RequestFactoryInterface
 */
final class RequestFactory implements \Psr\Http\Message\RequestFactoryInterface
{
    /**
     * Create a new request.
     *
     * @param string $method The HTTP method associated with the request
     * @param \Psr\Http\Message\UriInterface|string $uri The URI associated with the request
     * @return \Psr\Http\Message\RequestInterface
     * @throws \InvalidArgumentException If the URI is invalid
     */
    public function createRequest(string $method, $uri): \Psr\Http\Message\RequestInterface {}
}
