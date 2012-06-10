# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# These files are managed in the android-sync repo. Do not modify directly, or your changes will be lost.
SYNC_JAVA_FILES := sync/AlreadySyncingException.java sync/CollectionKeys.java sync/CommandProcessor.java sync/CommandRunner.java sync/CredentialsSource.java sync/crypto/CryptoException.java sync/crypto/CryptoInfo.java sync/crypto/HKDF.java sync/crypto/HMACVerificationException.java sync/crypto/KeyBundle.java sync/crypto/MissingCryptoInputException.java sync/crypto/NoKeyBundleException.java sync/crypto/PersistedCrypto5Keys.java sync/CryptoRecord.java sync/DelayedWorkTracker.java sync/delegates/ClientsDataDelegate.java sync/delegates/FreshStartDelegate.java sync/delegates/GlobalSessionCallback.java sync/delegates/InfoCollectionsDelegate.java sync/delegates/KeyUploadDelegate.java sync/delegates/MetaGlobalDelegate.java sync/delegates/WipeServerDelegate.java sync/EngineSettings.java sync/ExtendedJSONObject.java sync/GlobalSession.java sync/HTTPFailureException.java sync/InfoCollections.java sync/jpake/BigIntegerHelper.java sync/jpake/Gx3OrGx4IsZeroOrOneException.java sync/jpake/IncorrectZkpException.java sync/jpake/JPakeClient.java sync/jpake/JPakeCrypto.java sync/jpake/JPakeJson.java sync/jpake/JPakeNoActivePairingException.java sync/jpake/JPakeNumGenerator.java sync/jpake/JPakeNumGeneratorRandom.java sync/jpake/JPakeParty.java sync/jpake/JPakeRequest.java sync/jpake/JPakeRequestDelegate.java sync/jpake/JPakeResponse.java sync/jpake/stage/CompleteStage.java sync/jpake/stage/ComputeFinalStage.java sync/jpake/stage/ComputeKeyVerificationStage.java sync/jpake/stage/ComputeStepOneStage.java sync/jpake/stage/ComputeStepTwoStage.java sync/jpake/stage/DecryptDataStage.java sync/jpake/stage/DeleteChannel.java sync/jpake/stage/GetChannelStage.java sync/jpake/stage/GetRequestStage.java sync/jpake/stage/JPakeStage.java sync/jpake/stage/PutRequestStage.java sync/jpake/stage/VerifyPairingStage.java sync/jpake/Zkp.java sync/KeyBundleProvider.java sync/Logger.java sync/MetaGlobal.java sync/MetaGlobalException.java sync/MetaGlobalMissingEnginesException.java sync/MetaGlobalNotSetException.java sync/middleware/Crypto5MiddlewareRepository.java sync/middleware/Crypto5MiddlewareRepositorySession.java sync/middleware/MiddlewareRepository.java sync/middleware/MiddlewareRepositorySession.java sync/net/BaseResource.java sync/net/CompletedEntity.java sync/net/ConnectionMonitorThread.java sync/net/HandleProgressException.java sync/net/HttpResponseObserver.java sync/net/Resource.java sync/net/ResourceDelegate.java sync/net/SyncResourceDelegate.java sync/net/SyncResponse.java sync/net/SyncStorageCollectionRequest.java sync/net/SyncStorageCollectionRequestDelegate.java sync/net/SyncStorageRecordRequest.java sync/net/SyncStorageRequest.java sync/net/SyncStorageRequestDelegate.java sync/net/SyncStorageRequestIncrementalDelegate.java sync/net/SyncStorageResponse.java sync/net/TLSSocketFactory.java sync/net/WBOCollectionRequestDelegate.java sync/net/WBORequestDelegate.java sync/NoCollectionKeysSetException.java sync/NodeAuthenticationException.java sync/NonArrayJSONException.java sync/NonObjectJSONException.java sync/NullClusterURLException.java sync/PersistedMetaGlobal.java sync/PrefsSource.java sync/receivers/UpgradeReceiver.java sync/repositories/android/AndroidBrowserBookmarksDataAccessor.java sync/repositories/android/AndroidBrowserBookmarksRepository.java sync/repositories/android/AndroidBrowserBookmarksRepositorySession.java sync/repositories/android/AndroidBrowserHistoryDataAccessor.java sync/repositories/android/AndroidBrowserHistoryDataExtender.java sync/repositories/android/AndroidBrowserHistoryRepository.java sync/repositories/android/AndroidBrowserHistoryRepositorySession.java sync/repositories/android/AndroidBrowserRepository.java sync/repositories/android/AndroidBrowserRepositoryDataAccessor.java sync/repositories/android/AndroidBrowserRepositorySession.java sync/repositories/android/BookmarksDeletionManager.java sync/repositories/android/BookmarksInsertionManager.java sync/repositories/android/BrowserContractHelpers.java sync/repositories/android/CachedSQLiteOpenHelper.java sync/repositories/android/ClientsDatabase.java sync/repositories/android/ClientsDatabaseAccessor.java sync/repositories/android/FennecControlHelper.java sync/repositories/android/FennecTabsRepository.java sync/repositories/android/FormHistoryRepositorySession.java sync/repositories/android/PasswordsRepositorySession.java sync/repositories/android/RepoUtils.java sync/repositories/BookmarkNeedsReparentingException.java sync/repositories/BookmarksRepository.java sync/repositories/ConstrainedServer11Repository.java sync/repositories/delegates/DeferrableRepositorySessionCreationDelegate.java sync/repositories/delegates/DeferredRepositorySessionBeginDelegate.java sync/repositories/delegates/DeferredRepositorySessionFetchRecordsDelegate.java sync/repositories/delegates/DeferredRepositorySessionFinishDelegate.java sync/repositories/delegates/DeferredRepositorySessionStoreDelegate.java sync/repositories/delegates/RepositorySessionBeginDelegate.java sync/repositories/delegates/RepositorySessionCleanDelegate.java sync/repositories/delegates/RepositorySessionCreationDelegate.java sync/repositories/delegates/RepositorySessionFetchRecordsDelegate.java sync/repositories/delegates/RepositorySessionFinishDelegate.java sync/repositories/delegates/RepositorySessionGuidsSinceDelegate.java sync/repositories/delegates/RepositorySessionStoreDelegate.java sync/repositories/delegates/RepositorySessionWipeDelegate.java sync/repositories/domain/BookmarkRecord.java sync/repositories/domain/BookmarkRecordFactory.java sync/repositories/domain/ClientRecord.java sync/repositories/domain/ClientRecordFactory.java sync/repositories/domain/FormHistoryRecord.java sync/repositories/domain/HistoryRecord.java sync/repositories/domain/HistoryRecordFactory.java sync/repositories/domain/PasswordRecord.java sync/repositories/domain/Record.java sync/repositories/domain/TabsRecord.java sync/repositories/domain/VersionConstants.java sync/repositories/FetchFailedException.java sync/repositories/HashSetStoreTracker.java sync/repositories/HistoryRepository.java sync/repositories/IdentityRecordFactory.java sync/repositories/InactiveSessionException.java sync/repositories/InvalidBookmarkTypeException.java sync/repositories/InvalidRequestException.java sync/repositories/InvalidSessionTransitionException.java sync/repositories/MultipleRecordsForGuidException.java sync/repositories/NoContentProviderException.java sync/repositories/NoGuidForIdException.java sync/repositories/NoStoreDelegateException.java sync/repositories/NullCursorException.java sync/repositories/ParentNotFoundException.java sync/repositories/ProfileDatabaseException.java sync/repositories/RecordFactory.java sync/repositories/RecordFilter.java sync/repositories/Repository.java sync/repositories/RepositorySession.java sync/repositories/RepositorySessionBundle.java sync/repositories/Server11Repository.java sync/repositories/Server11RepositorySession.java sync/repositories/StoreFailedException.java sync/repositories/StoreTracker.java sync/repositories/StoreTrackingRepositorySession.java sync/Server11PreviousPostFailedException.java sync/Server11RecordPostFailedException.java sync/setup/activities/AccountActivity.java sync/setup/activities/ActivityUtils.java sync/setup/activities/ClientRecordArrayAdapter.java sync/setup/activities/RedirectToSetupActivity.java sync/setup/activities/SendTabActivity.java sync/setup/activities/SetupFailureActivity.java sync/setup/activities/SetupSuccessActivity.java sync/setup/activities/SetupSyncActivity.java sync/setup/auth/AccountAuthenticator.java sync/setup/auth/AuthenticateAccountStage.java sync/setup/auth/AuthenticationResult.java sync/setup/auth/AuthenticatorStage.java sync/setup/auth/EnsureUserExistenceStage.java sync/setup/auth/FetchUserNodeStage.java sync/setup/Constants.java sync/setup/InvalidSyncKeyException.java sync/setup/SyncAccounts.java sync/setup/SyncAuthenticatorService.java sync/stage/AbstractNonRepositorySyncStage.java sync/stage/AndroidBrowserBookmarksServerSyncStage.java sync/stage/AndroidBrowserHistoryServerSyncStage.java sync/stage/CheckPreconditionsStage.java sync/stage/CompletedStage.java sync/stage/EnsureClusterURLStage.java sync/stage/EnsureCrypto5KeysStage.java sync/stage/FennecTabsServerSyncStage.java sync/stage/FetchInfoCollectionsStage.java sync/stage/FetchMetaGlobalStage.java sync/stage/FormHistoryServerSyncStage.java sync/stage/GlobalSyncStage.java sync/stage/NoSuchStageException.java sync/stage/NoSyncIDException.java sync/stage/PasswordsServerSyncStage.java sync/stage/ServerSyncStage.java sync/stage/SyncClientsEngineStage.java sync/stage/UploadMetaGlobalStage.java sync/StubActivity.java sync/syncadapter/SyncAdapter.java sync/syncadapter/SyncService.java sync/SyncConfiguration.java sync/SyncConfigurationException.java sync/SyncException.java sync/synchronizer/ConcurrentRecordConsumer.java sync/synchronizer/RecordConsumer.java sync/synchronizer/RecordsChannel.java sync/synchronizer/RecordsChannelDelegate.java sync/synchronizer/RecordsConsumerDelegate.java sync/synchronizer/SerialRecordConsumer.java sync/synchronizer/ServerLocalSynchronizer.java sync/synchronizer/ServerLocalSynchronizerSession.java sync/synchronizer/SessionNotBegunException.java sync/synchronizer/Synchronizer.java sync/synchronizer/SynchronizerDelegate.java sync/synchronizer/SynchronizerSession.java sync/synchronizer/SynchronizerSessionDelegate.java sync/synchronizer/UnbundleError.java sync/synchronizer/UnexpectedSessionException.java sync/SynchronizerConfiguration.java sync/ThreadPool.java sync/UnexpectedJSONException.java sync/UnknownSynchronizerConfigurationVersionException.java sync/Utils.java
SYNC_PP_JAVA_FILES := sync/GlobalConstants.java
SYNC_THIRDPARTY_JAVA_FILES := httpclientandroidlib/androidextra/HttpClientAndroidLog.java httpclientandroidlib/annotation/GuardedBy.java httpclientandroidlib/annotation/Immutable.java httpclientandroidlib/annotation/NotThreadSafe.java httpclientandroidlib/annotation/ThreadSafe.java httpclientandroidlib/auth/AUTH.java httpclientandroidlib/auth/AuthenticationException.java httpclientandroidlib/auth/AuthScheme.java httpclientandroidlib/auth/AuthSchemeFactory.java httpclientandroidlib/auth/AuthSchemeRegistry.java httpclientandroidlib/auth/AuthScope.java httpclientandroidlib/auth/AuthState.java httpclientandroidlib/auth/BasicUserPrincipal.java httpclientandroidlib/auth/ContextAwareAuthScheme.java httpclientandroidlib/auth/Credentials.java httpclientandroidlib/auth/InvalidCredentialsException.java httpclientandroidlib/auth/MalformedChallengeException.java httpclientandroidlib/auth/NTCredentials.java httpclientandroidlib/auth/NTUserPrincipal.java httpclientandroidlib/auth/params/AuthParamBean.java httpclientandroidlib/auth/params/AuthParams.java httpclientandroidlib/auth/params/AuthPNames.java httpclientandroidlib/auth/UsernamePasswordCredentials.java httpclientandroidlib/client/AuthCache.java httpclientandroidlib/client/AuthenticationHandler.java httpclientandroidlib/client/CircularRedirectException.java httpclientandroidlib/client/ClientProtocolException.java httpclientandroidlib/client/CookieStore.java httpclientandroidlib/client/CredentialsProvider.java httpclientandroidlib/client/entity/DecompressingEntity.java httpclientandroidlib/client/entity/DeflateDecompressingEntity.java httpclientandroidlib/client/entity/GzipDecompressingEntity.java httpclientandroidlib/client/entity/UrlEncodedFormEntity.java httpclientandroidlib/client/HttpClient.java httpclientandroidlib/client/HttpRequestRetryHandler.java httpclientandroidlib/client/HttpResponseException.java httpclientandroidlib/client/methods/AbortableHttpRequest.java httpclientandroidlib/client/methods/HttpDelete.java httpclientandroidlib/client/methods/HttpEntityEnclosingRequestBase.java httpclientandroidlib/client/methods/HttpGet.java httpclientandroidlib/client/methods/HttpHead.java httpclientandroidlib/client/methods/HttpOptions.java httpclientandroidlib/client/methods/HttpPost.java httpclientandroidlib/client/methods/HttpPut.java httpclientandroidlib/client/methods/HttpRequestBase.java httpclientandroidlib/client/methods/HttpTrace.java httpclientandroidlib/client/methods/HttpUriRequest.java httpclientandroidlib/client/NonRepeatableRequestException.java httpclientandroidlib/client/params/AllClientPNames.java httpclientandroidlib/client/params/AuthPolicy.java httpclientandroidlib/client/params/ClientParamBean.java httpclientandroidlib/client/params/ClientPNames.java httpclientandroidlib/client/params/CookiePolicy.java httpclientandroidlib/client/params/HttpClientParams.java httpclientandroidlib/client/protocol/ClientContext.java httpclientandroidlib/client/protocol/ClientContextConfigurer.java httpclientandroidlib/client/protocol/RequestAcceptEncoding.java httpclientandroidlib/client/protocol/RequestAddCookies.java httpclientandroidlib/client/protocol/RequestAuthCache.java httpclientandroidlib/client/protocol/RequestClientConnControl.java httpclientandroidlib/client/protocol/RequestDefaultHeaders.java httpclientandroidlib/client/protocol/RequestProxyAuthentication.java httpclientandroidlib/client/protocol/RequestTargetAuthentication.java httpclientandroidlib/client/protocol/ResponseAuthCache.java httpclientandroidlib/client/protocol/ResponseContentEncoding.java httpclientandroidlib/client/protocol/ResponseProcessCookies.java httpclientandroidlib/client/RedirectException.java httpclientandroidlib/client/RedirectHandler.java httpclientandroidlib/client/RedirectStrategy.java httpclientandroidlib/client/RequestDirector.java httpclientandroidlib/client/ResponseHandler.java httpclientandroidlib/client/UserTokenHandler.java httpclientandroidlib/client/utils/CloneUtils.java httpclientandroidlib/client/utils/Idn.java httpclientandroidlib/client/utils/JdkIdn.java httpclientandroidlib/client/utils/Punycode.java httpclientandroidlib/client/utils/Rfc3492Idn.java httpclientandroidlib/client/utils/URIUtils.java httpclientandroidlib/client/utils/URLEncodedUtils.java httpclientandroidlib/conn/BasicEofSensorWatcher.java httpclientandroidlib/conn/BasicManagedEntity.java httpclientandroidlib/conn/ClientConnectionManager.java httpclientandroidlib/conn/ClientConnectionManagerFactory.java httpclientandroidlib/conn/ClientConnectionOperator.java httpclientandroidlib/conn/ClientConnectionRequest.java httpclientandroidlib/conn/ConnectionKeepAliveStrategy.java httpclientandroidlib/conn/ConnectionPoolTimeoutException.java httpclientandroidlib/conn/ConnectionReleaseTrigger.java httpclientandroidlib/conn/ConnectTimeoutException.java httpclientandroidlib/conn/EofSensorInputStream.java httpclientandroidlib/conn/EofSensorWatcher.java httpclientandroidlib/conn/HttpHostConnectException.java httpclientandroidlib/conn/HttpRoutedConnection.java httpclientandroidlib/conn/ManagedClientConnection.java httpclientandroidlib/conn/MultihomePlainSocketFactory.java httpclientandroidlib/conn/OperatedClientConnection.java httpclientandroidlib/conn/params/ConnConnectionParamBean.java httpclientandroidlib/conn/params/ConnConnectionPNames.java httpclientandroidlib/conn/params/ConnManagerParamBean.java httpclientandroidlib/conn/params/ConnManagerParams.java httpclientandroidlib/conn/params/ConnManagerPNames.java httpclientandroidlib/conn/params/ConnPerRoute.java httpclientandroidlib/conn/params/ConnPerRouteBean.java httpclientandroidlib/conn/params/ConnRouteParamBean.java httpclientandroidlib/conn/params/ConnRouteParams.java httpclientandroidlib/conn/params/ConnRoutePNames.java httpclientandroidlib/conn/routing/BasicRouteDirector.java httpclientandroidlib/conn/routing/HttpRoute.java httpclientandroidlib/conn/routing/HttpRouteDirector.java httpclientandroidlib/conn/routing/HttpRoutePlanner.java httpclientandroidlib/conn/routing/RouteInfo.java httpclientandroidlib/conn/routing/RouteTracker.java httpclientandroidlib/conn/scheme/HostNameResolver.java httpclientandroidlib/conn/scheme/LayeredSchemeSocketFactory.java httpclientandroidlib/conn/scheme/LayeredSchemeSocketFactoryAdaptor.java httpclientandroidlib/conn/scheme/LayeredSocketFactory.java httpclientandroidlib/conn/scheme/LayeredSocketFactoryAdaptor.java httpclientandroidlib/conn/scheme/PlainSocketFactory.java httpclientandroidlib/conn/scheme/Scheme.java httpclientandroidlib/conn/scheme/SchemeRegistry.java httpclientandroidlib/conn/scheme/SchemeSocketFactory.java httpclientandroidlib/conn/scheme/SchemeSocketFactoryAdaptor.java httpclientandroidlib/conn/scheme/SocketFactory.java httpclientandroidlib/conn/scheme/SocketFactoryAdaptor.java httpclientandroidlib/conn/ssl/AbstractVerifier.java httpclientandroidlib/conn/ssl/AllowAllHostnameVerifier.java httpclientandroidlib/conn/ssl/BrowserCompatHostnameVerifier.java httpclientandroidlib/conn/ssl/SSLSocketFactory.java httpclientandroidlib/conn/ssl/StrictHostnameVerifier.java httpclientandroidlib/conn/ssl/TrustManagerDecorator.java httpclientandroidlib/conn/ssl/TrustSelfSignedStrategy.java httpclientandroidlib/conn/ssl/TrustStrategy.java httpclientandroidlib/conn/ssl/X509HostnameVerifier.java httpclientandroidlib/conn/util/InetAddressUtils.java httpclientandroidlib/ConnectionClosedException.java httpclientandroidlib/ConnectionReuseStrategy.java httpclientandroidlib/cookie/ClientCookie.java httpclientandroidlib/cookie/Cookie.java httpclientandroidlib/cookie/CookieAttributeHandler.java httpclientandroidlib/cookie/CookieIdentityComparator.java httpclientandroidlib/cookie/CookieOrigin.java httpclientandroidlib/cookie/CookiePathComparator.java httpclientandroidlib/cookie/CookieRestrictionViolationException.java httpclientandroidlib/cookie/CookieSpec.java httpclientandroidlib/cookie/CookieSpecFactory.java httpclientandroidlib/cookie/CookieSpecRegistry.java httpclientandroidlib/cookie/MalformedCookieException.java httpclientandroidlib/cookie/params/CookieSpecParamBean.java httpclientandroidlib/cookie/params/CookieSpecPNames.java httpclientandroidlib/cookie/SetCookie.java httpclientandroidlib/cookie/SetCookie2.java httpclientandroidlib/cookie/SM.java httpclientandroidlib/entity/AbstractHttpEntity.java httpclientandroidlib/entity/BasicHttpEntity.java httpclientandroidlib/entity/BufferedHttpEntity.java httpclientandroidlib/entity/ByteArrayEntity.java httpclientandroidlib/entity/ContentLengthStrategy.java httpclientandroidlib/entity/ContentProducer.java httpclientandroidlib/entity/EntityTemplate.java httpclientandroidlib/entity/FileEntity.java httpclientandroidlib/entity/HttpEntityWrapper.java httpclientandroidlib/entity/InputStreamEntity.java httpclientandroidlib/entity/SerializableEntity.java httpclientandroidlib/entity/StringEntity.java httpclientandroidlib/FormattedHeader.java httpclientandroidlib/Header.java httpclientandroidlib/HeaderElement.java httpclientandroidlib/HeaderElementIterator.java httpclientandroidlib/HeaderIterator.java httpclientandroidlib/HttpClientConnection.java httpclientandroidlib/HttpConnection.java httpclientandroidlib/HttpConnectionMetrics.java httpclientandroidlib/HttpEntity.java httpclientandroidlib/HttpEntityEnclosingRequest.java httpclientandroidlib/HttpException.java httpclientandroidlib/HttpHeaders.java httpclientandroidlib/HttpHost.java httpclientandroidlib/HttpInetConnection.java httpclientandroidlib/HttpMessage.java httpclientandroidlib/HttpRequest.java httpclientandroidlib/HttpRequestFactory.java httpclientandroidlib/HttpRequestInterceptor.java httpclientandroidlib/HttpResponse.java httpclientandroidlib/HttpResponseFactory.java httpclientandroidlib/HttpResponseInterceptor.java httpclientandroidlib/HttpServerConnection.java httpclientandroidlib/HttpStatus.java httpclientandroidlib/HttpVersion.java httpclientandroidlib/impl/AbstractHttpClientConnection.java httpclientandroidlib/impl/AbstractHttpServerConnection.java httpclientandroidlib/impl/auth/AuthSchemeBase.java httpclientandroidlib/impl/auth/BasicScheme.java httpclientandroidlib/impl/auth/BasicSchemeFactory.java httpclientandroidlib/impl/auth/DigestScheme.java httpclientandroidlib/impl/auth/DigestSchemeFactory.java httpclientandroidlib/impl/auth/NTLMEngine.java httpclientandroidlib/impl/auth/NTLMEngineException.java httpclientandroidlib/impl/auth/NTLMEngineImpl.java httpclientandroidlib/impl/auth/NTLMScheme.java httpclientandroidlib/impl/auth/NTLMSchemeFactory.java httpclientandroidlib/impl/auth/RFC2617Scheme.java httpclientandroidlib/impl/auth/SpnegoTokenGenerator.java httpclientandroidlib/impl/auth/UnsupportedDigestAlgorithmException.java httpclientandroidlib/impl/client/AbstractAuthenticationHandler.java httpclientandroidlib/impl/client/AbstractHttpClient.java httpclientandroidlib/impl/client/BasicAuthCache.java httpclientandroidlib/impl/client/BasicCookieStore.java httpclientandroidlib/impl/client/BasicCredentialsProvider.java httpclientandroidlib/impl/client/BasicResponseHandler.java httpclientandroidlib/impl/client/ClientParamsStack.java httpclientandroidlib/impl/client/ContentEncodingHttpClient.java httpclientandroidlib/impl/client/DefaultConnectionKeepAliveStrategy.java httpclientandroidlib/impl/client/DefaultHttpClient.java httpclientandroidlib/impl/client/DefaultHttpRequestRetryHandler.java httpclientandroidlib/impl/client/DefaultProxyAuthenticationHandler.java httpclientandroidlib/impl/client/DefaultRedirectHandler.java httpclientandroidlib/impl/client/DefaultRedirectStrategy.java httpclientandroidlib/impl/client/DefaultRedirectStrategyAdaptor.java httpclientandroidlib/impl/client/DefaultRequestDirector.java httpclientandroidlib/impl/client/DefaultTargetAuthenticationHandler.java httpclientandroidlib/impl/client/DefaultUserTokenHandler.java httpclientandroidlib/impl/client/EntityEnclosingRequestWrapper.java httpclientandroidlib/impl/client/RedirectLocations.java httpclientandroidlib/impl/client/RequestWrapper.java httpclientandroidlib/impl/client/RoutedRequest.java httpclientandroidlib/impl/client/TunnelRefusedException.java httpclientandroidlib/impl/conn/AbstractClientConnAdapter.java httpclientandroidlib/impl/conn/AbstractPooledConnAdapter.java httpclientandroidlib/impl/conn/AbstractPoolEntry.java httpclientandroidlib/impl/conn/ConnectionShutdownException.java httpclientandroidlib/impl/conn/DefaultClientConnection.java httpclientandroidlib/impl/conn/DefaultClientConnectionOperator.java httpclientandroidlib/impl/conn/DefaultHttpRoutePlanner.java httpclientandroidlib/impl/conn/DefaultResponseParser.java httpclientandroidlib/impl/conn/HttpInetSocketAddress.java httpclientandroidlib/impl/conn/IdleConnectionHandler.java httpclientandroidlib/impl/conn/LoggingSessionInputBuffer.java httpclientandroidlib/impl/conn/LoggingSessionOutputBuffer.java httpclientandroidlib/impl/conn/ProxySelectorRoutePlanner.java httpclientandroidlib/impl/conn/SchemeRegistryFactory.java httpclientandroidlib/impl/conn/SingleClientConnManager.java httpclientandroidlib/impl/conn/tsccm/AbstractConnPool.java httpclientandroidlib/impl/conn/tsccm/BasicPooledConnAdapter.java httpclientandroidlib/impl/conn/tsccm/BasicPoolEntry.java httpclientandroidlib/impl/conn/tsccm/BasicPoolEntryRef.java httpclientandroidlib/impl/conn/tsccm/ConnPoolByRoute.java httpclientandroidlib/impl/conn/tsccm/PoolEntryRequest.java httpclientandroidlib/impl/conn/tsccm/RefQueueHandler.java httpclientandroidlib/impl/conn/tsccm/RefQueueWorker.java httpclientandroidlib/impl/conn/tsccm/RouteSpecificPool.java httpclientandroidlib/impl/conn/tsccm/ThreadSafeClientConnManager.java httpclientandroidlib/impl/conn/tsccm/WaitingThread.java httpclientandroidlib/impl/conn/tsccm/WaitingThreadAborter.java httpclientandroidlib/impl/conn/Wire.java httpclientandroidlib/impl/cookie/AbstractCookieAttributeHandler.java httpclientandroidlib/impl/cookie/AbstractCookieSpec.java httpclientandroidlib/impl/cookie/BasicClientCookie.java httpclientandroidlib/impl/cookie/BasicClientCookie2.java httpclientandroidlib/impl/cookie/BasicCommentHandler.java httpclientandroidlib/impl/cookie/BasicDomainHandler.java httpclientandroidlib/impl/cookie/BasicExpiresHandler.java httpclientandroidlib/impl/cookie/BasicMaxAgeHandler.java httpclientandroidlib/impl/cookie/BasicPathHandler.java httpclientandroidlib/impl/cookie/BasicSecureHandler.java httpclientandroidlib/impl/cookie/BestMatchSpec.java httpclientandroidlib/impl/cookie/BestMatchSpecFactory.java httpclientandroidlib/impl/cookie/BrowserCompatSpec.java httpclientandroidlib/impl/cookie/BrowserCompatSpecFactory.java httpclientandroidlib/impl/cookie/CookieSpecBase.java httpclientandroidlib/impl/cookie/DateParseException.java httpclientandroidlib/impl/cookie/DateUtils.java httpclientandroidlib/impl/cookie/IgnoreSpec.java httpclientandroidlib/impl/cookie/IgnoreSpecFactory.java httpclientandroidlib/impl/cookie/NetscapeDomainHandler.java httpclientandroidlib/impl/cookie/NetscapeDraftHeaderParser.java httpclientandroidlib/impl/cookie/NetscapeDraftSpec.java httpclientandroidlib/impl/cookie/NetscapeDraftSpecFactory.java httpclientandroidlib/impl/cookie/PublicSuffixFilter.java httpclientandroidlib/impl/cookie/PublicSuffixListParser.java httpclientandroidlib/impl/cookie/RFC2109DomainHandler.java httpclientandroidlib/impl/cookie/RFC2109Spec.java httpclientandroidlib/impl/cookie/RFC2109SpecFactory.java httpclientandroidlib/impl/cookie/RFC2109VersionHandler.java httpclientandroidlib/impl/cookie/RFC2965CommentUrlAttributeHandler.java httpclientandroidlib/impl/cookie/RFC2965DiscardAttributeHandler.java httpclientandroidlib/impl/cookie/RFC2965DomainAttributeHandler.java httpclientandroidlib/impl/cookie/RFC2965PortAttributeHandler.java httpclientandroidlib/impl/cookie/RFC2965Spec.java httpclientandroidlib/impl/cookie/RFC2965SpecFactory.java httpclientandroidlib/impl/cookie/RFC2965VersionAttributeHandler.java httpclientandroidlib/impl/DefaultConnectionReuseStrategy.java httpclientandroidlib/impl/DefaultHttpClientConnection.java httpclientandroidlib/impl/DefaultHttpRequestFactory.java httpclientandroidlib/impl/DefaultHttpResponseFactory.java httpclientandroidlib/impl/DefaultHttpServerConnection.java httpclientandroidlib/impl/EnglishReasonPhraseCatalog.java httpclientandroidlib/impl/entity/EntityDeserializer.java httpclientandroidlib/impl/entity/EntitySerializer.java httpclientandroidlib/impl/entity/LaxContentLengthStrategy.java httpclientandroidlib/impl/entity/StrictContentLengthStrategy.java httpclientandroidlib/impl/HttpConnectionMetricsImpl.java httpclientandroidlib/impl/io/AbstractMessageParser.java httpclientandroidlib/impl/io/AbstractMessageWriter.java httpclientandroidlib/impl/io/AbstractSessionInputBuffer.java httpclientandroidlib/impl/io/AbstractSessionOutputBuffer.java httpclientandroidlib/impl/io/ChunkedInputStream.java httpclientandroidlib/impl/io/ChunkedOutputStream.java httpclientandroidlib/impl/io/ContentLengthInputStream.java httpclientandroidlib/impl/io/ContentLengthOutputStream.java httpclientandroidlib/impl/io/HttpRequestParser.java httpclientandroidlib/impl/io/HttpRequestWriter.java httpclientandroidlib/impl/io/HttpResponseParser.java httpclientandroidlib/impl/io/HttpResponseWriter.java httpclientandroidlib/impl/io/HttpTransportMetricsImpl.java httpclientandroidlib/impl/io/IdentityInputStream.java httpclientandroidlib/impl/io/IdentityOutputStream.java httpclientandroidlib/impl/io/SocketInputBuffer.java httpclientandroidlib/impl/io/SocketOutputBuffer.java httpclientandroidlib/impl/NoConnectionReuseStrategy.java httpclientandroidlib/impl/SocketHttpClientConnection.java httpclientandroidlib/impl/SocketHttpServerConnection.java httpclientandroidlib/io/BufferInfo.java httpclientandroidlib/io/EofSensor.java httpclientandroidlib/io/HttpMessageParser.java httpclientandroidlib/io/HttpMessageWriter.java httpclientandroidlib/io/HttpTransportMetrics.java httpclientandroidlib/io/SessionInputBuffer.java httpclientandroidlib/io/SessionOutputBuffer.java httpclientandroidlib/MalformedChunkCodingException.java httpclientandroidlib/message/AbstractHttpMessage.java httpclientandroidlib/message/BasicHeader.java httpclientandroidlib/message/BasicHeaderElement.java httpclientandroidlib/message/BasicHeaderElementIterator.java httpclientandroidlib/message/BasicHeaderIterator.java httpclientandroidlib/message/BasicHeaderValueFormatter.java httpclientandroidlib/message/BasicHeaderValueParser.java httpclientandroidlib/message/BasicHttpEntityEnclosingRequest.java httpclientandroidlib/message/BasicHttpRequest.java httpclientandroidlib/message/BasicHttpResponse.java httpclientandroidlib/message/BasicLineFormatter.java httpclientandroidlib/message/BasicLineParser.java httpclientandroidlib/message/BasicListHeaderIterator.java httpclientandroidlib/message/BasicNameValuePair.java httpclientandroidlib/message/BasicRequestLine.java httpclientandroidlib/message/BasicStatusLine.java httpclientandroidlib/message/BasicTokenIterator.java httpclientandroidlib/message/BufferedHeader.java httpclientandroidlib/message/HeaderGroup.java httpclientandroidlib/message/HeaderValueFormatter.java httpclientandroidlib/message/HeaderValueParser.java httpclientandroidlib/message/LineFormatter.java httpclientandroidlib/message/LineParser.java httpclientandroidlib/message/ParserCursor.java httpclientandroidlib/MethodNotSupportedException.java httpclientandroidlib/NameValuePair.java httpclientandroidlib/NoHttpResponseException.java httpclientandroidlib/params/AbstractHttpParams.java httpclientandroidlib/params/BasicHttpParams.java httpclientandroidlib/params/CoreConnectionPNames.java httpclientandroidlib/params/CoreProtocolPNames.java httpclientandroidlib/params/DefaultedHttpParams.java httpclientandroidlib/params/HttpAbstractParamBean.java httpclientandroidlib/params/HttpConnectionParamBean.java httpclientandroidlib/params/HttpConnectionParams.java httpclientandroidlib/params/HttpParams.java httpclientandroidlib/params/HttpProtocolParamBean.java httpclientandroidlib/params/HttpProtocolParams.java httpclientandroidlib/params/SyncBasicHttpParams.java httpclientandroidlib/ParseException.java httpclientandroidlib/protocol/BasicHttpContext.java httpclientandroidlib/protocol/BasicHttpProcessor.java httpclientandroidlib/protocol/DefaultedHttpContext.java httpclientandroidlib/protocol/ExecutionContext.java httpclientandroidlib/protocol/HTTP.java httpclientandroidlib/protocol/HttpContext.java httpclientandroidlib/protocol/HttpDateGenerator.java httpclientandroidlib/protocol/HttpExpectationVerifier.java httpclientandroidlib/protocol/HttpProcessor.java httpclientandroidlib/protocol/HttpRequestExecutor.java httpclientandroidlib/protocol/HttpRequestHandler.java httpclientandroidlib/protocol/HttpRequestHandlerRegistry.java httpclientandroidlib/protocol/HttpRequestHandlerResolver.java httpclientandroidlib/protocol/HttpRequestInterceptorList.java httpclientandroidlib/protocol/HttpResponseInterceptorList.java httpclientandroidlib/protocol/HttpService.java httpclientandroidlib/protocol/ImmutableHttpProcessor.java httpclientandroidlib/protocol/RequestConnControl.java httpclientandroidlib/protocol/RequestContent.java httpclientandroidlib/protocol/RequestDate.java httpclientandroidlib/protocol/RequestExpectContinue.java httpclientandroidlib/protocol/RequestTargetHost.java httpclientandroidlib/protocol/RequestUserAgent.java httpclientandroidlib/protocol/ResponseConnControl.java httpclientandroidlib/protocol/ResponseContent.java httpclientandroidlib/protocol/ResponseDate.java httpclientandroidlib/protocol/ResponseServer.java httpclientandroidlib/protocol/SyncBasicHttpContext.java httpclientandroidlib/protocol/UriPatternMatcher.java httpclientandroidlib/ProtocolException.java httpclientandroidlib/ProtocolVersion.java httpclientandroidlib/ReasonPhraseCatalog.java httpclientandroidlib/RequestLine.java httpclientandroidlib/StatusLine.java httpclientandroidlib/TokenIterator.java httpclientandroidlib/TruncatedChunkException.java httpclientandroidlib/UnsupportedHttpVersionException.java httpclientandroidlib/util/ByteArrayBuffer.java httpclientandroidlib/util/CharArrayBuffer.java httpclientandroidlib/util/EncodingUtils.java httpclientandroidlib/util/EntityUtils.java httpclientandroidlib/util/ExceptionUtils.java httpclientandroidlib/util/LangUtils.java httpclientandroidlib/util/VersionInfo.java json-simple/ItemList.java json-simple/JSONArray.java json-simple/JSONAware.java json-simple/JSONObject.java json-simple/JSONStreamAware.java json-simple/JSONValue.java json-simple/parser/ContainerFactory.java json-simple/parser/ContentHandler.java json-simple/parser/JSONParser.java json-simple/parser/ParseException.java json-simple/parser/Yylex.java json-simple/parser/Yytoken.java apache/commons/codec/binary/Base32.java apache/commons/codec/binary/Base32InputStream.java apache/commons/codec/binary/Base32OutputStream.java apache/commons/codec/binary/Base64.java apache/commons/codec/binary/Base64InputStream.java apache/commons/codec/binary/Base64OutputStream.java apache/commons/codec/binary/BaseNCodec.java apache/commons/codec/binary/BaseNCodecInputStream.java apache/commons/codec/binary/BaseNCodecOutputStream.java apache/commons/codec/binary/BinaryCodec.java apache/commons/codec/binary/Hex.java apache/commons/codec/binary/StringUtils.java apache/commons/codec/BinaryDecoder.java apache/commons/codec/BinaryEncoder.java apache/commons/codec/CharEncoding.java apache/commons/codec/Decoder.java apache/commons/codec/DecoderException.java apache/commons/codec/digest/DigestUtils.java apache/commons/codec/Encoder.java apache/commons/codec/EncoderException.java apache/commons/codec/language/AbstractCaverphone.java apache/commons/codec/language/Caverphone.java apache/commons/codec/language/Caverphone1.java apache/commons/codec/language/Caverphone2.java apache/commons/codec/language/ColognePhonetic.java apache/commons/codec/language/DoubleMetaphone.java apache/commons/codec/language/Metaphone.java apache/commons/codec/language/RefinedSoundex.java apache/commons/codec/language/Soundex.java apache/commons/codec/language/SoundexUtils.java apache/commons/codec/net/BCodec.java apache/commons/codec/net/QCodec.java apache/commons/codec/net/QuotedPrintableCodec.java apache/commons/codec/net/RFC1522Codec.java apache/commons/codec/net/URLCodec.java apache/commons/codec/net/Utils.java apache/commons/codec/StringDecoder.java apache/commons/codec/StringEncoder.java apache/commons/codec/StringEncoderComparator.java
SYNC_RES_DRAWABLE := mobile/android/base/resources/drawable/desktop.png mobile/android/base/resources/drawable/mobile.png mobile/android/base/resources/drawable/pin_background.xml mobile/android/base/resources/drawable/sync_ic_launcher.png
SYNC_RES_DRAWABLE_LDPI := mobile/android/base/resources/drawable-ldpi/sync_ic_launcher.png
SYNC_RES_DRAWABLE_MDPI := mobile/android/base/resources/drawable-mdpi/sync_ic_launcher.png
SYNC_RES_DRAWABLE_HDPI := mobile/android/base/resources/drawable-hdpi/sync_ic_launcher.png
SYNC_RES_LAYOUT := res/layout/sync_account.xml res/layout/sync_custom_popup.xml res/layout/sync_list_item.xml res/layout/sync_redirect_to_setup.xml res/layout/sync_send_tab.xml res/layout/sync_setup.xml res/layout/sync_setup_failure.xml res/layout/sync_setup_jpake_waiting.xml res/layout/sync_setup_nointernet.xml res/layout/sync_setup_pair.xml res/layout/sync_setup_success.xml res/layout/sync_stub.xml
SYNC_RES_VALUES := res/values/sync_styles.xml
