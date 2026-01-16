<?php
/**
 * Signalforge HTTP Extension
 * ServerRequestFactory.stub.php - IDE stub for ServerRequestFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant server request factory.
 *
 * Creates ServerRequestInterface instances.
 *
 * @final
 * @implements \Psr\Http\Message\ServerRequestFactoryInterface
 */
final class ServerRequestFactory implements \Psr\Http\Message\ServerRequestFactoryInterface
{
    /**
     * Create a new server request.
     *
     * @param string $method The HTTP method associated with the request
     * @param \Psr\Http\Message\UriInterface|string $uri The URI associated with the request
     * @param array $serverParams Array of SAPI parameters (typically from $_SERVER)
     * @return \Psr\Http\Message\ServerRequestInterface
     * @throws \InvalidArgumentException If the URI is invalid
     */
    public function createServerRequest(
        string $method,
        $uri,
        array $serverParams = []
    ): \Psr\Http\Message\ServerRequestInterface {}
}
