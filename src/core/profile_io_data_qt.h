/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PROFILE_IO_DATA_QT_H
#define PROFILE_IO_DATA_QT_H

#include "profile_adapter.h"
#include "content/public/browser/browsing_data_remover.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "extensions/buildflags/buildflags.h"
#include "net/proxy_config_monitor.h"
#include "services/network/cookie_settings.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtCore/QMutex>

namespace net {
class ClientCertStore;
class ProxyConfigService;
}

namespace extensions {
class ExtensionSystemQt;
}

namespace QtWebEngineCore {

struct ClientCertificateStoreData;
class ProfileIODataQt;
class ProfileQt;


class BrowsingDataRemoverObserverQt : public content::BrowsingDataRemover::Observer {
public:
    BrowsingDataRemoverObserverQt(ProfileIODataQt *profileIOData);

    void OnBrowsingDataRemoverDone() override;

private:
    ProfileIODataQt *m_profileIOData;
};


// ProfileIOData contains data that lives on the IOthread
// we still use shared memebers and use mutex which breaks
// idea for this object, but this is wip.

class ProfileIODataQt {

public:
    ProfileIODataQt(ProfileQt *profile); // runs on ui thread
    virtual ~ProfileIODataQt();

    QPointer<ProfileAdapter> profileAdapter();
    content::ResourceContext *resourceContext();
    net::URLRequestContext *urlRequestContext();
#if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions::ExtensionSystemQt* GetExtensionSystem();
#endif // BUILDFLAG(ENABLE_EXTENSIONS)

    ProtocolHandlerRegistry::IOThreadDelegate *protocolHandlerRegistryIOThreadDelegate()
    {
        return m_protocolHandlerRegistryIOThreadDelegate.get();
    }

    void initializeOnIOThread();
    void initializeOnUIThread(); // runs on ui thread
    void shutdownOnUIThread(); // runs on ui thread

    void cancelAllUrlRequests();
    void generateAllStorage();
    void regenerateJobFactory();
    bool canSetCookie(const QUrl &firstPartyUrl, const QByteArray &cookieLine, const QUrl &url) const;
    bool canGetCookies(const QUrl &firstPartyUrl, const QUrl &url) const;
    void setGlobalCertificateVerification();

    // Used in NetworkDelegateQt::OnBeforeURLRequest.
    bool isInterceptorDeprecated() const; // Remove for Qt6
    QWebEngineUrlRequestInterceptor *acquireInterceptor(); // Remove for Qt6
    void releaseInterceptor();
    QWebEngineUrlRequestInterceptor *requestInterceptor();

    void setRequestContextData(content::ProtocolHandlerMap *protocolHandlers,
                               content::URLRequestInterceptorScopedVector request_interceptors);
    void setFullConfiguration(); // runs on ui thread
    void updateStorageSettings(); // runs on ui thread
    void updateUserAgent(); // runs on ui thread
    void updateCookieStore(); // runs on ui thread
    void updateHttpCache(); // runs on ui thread
    void updateJobFactory(); // runs on ui thread
    void updateRequestInterceptor(); // runs on ui thread
    void requestStorageGeneration(); //runs on ui thread
    void createProxyConfig(); //runs on ui thread
    void updateUsedForGlobalCertificateVerification(); // runs on ui thread
    bool hasPageInterceptors();

    network::mojom::NetworkContextParamsPtr CreateNetworkContextParams();

#if QT_CONFIG(ssl)
    ClientCertificateStoreData *clientCertificateStoreData();
#endif
    std::unique_ptr<net::ClientCertStore> CreateClientCertStore();
    static ProfileIODataQt *FromBrowserContext(content::BrowserContext *browser_context);
    static ProfileIODataQt *FromResourceContext(content::ResourceContext *resource_context);

    base::WeakPtr<ProfileIODataQt> getWeakPtrOnIOThread();
    base::WeakPtr<ProfileIODataQt> getWeakPtrOnUIThread();

    CookieMonsterDelegateQt *cookieDelegate() const { return m_cookieDelegate.get(); }

private:
    void removeBrowsingDataRemoverObserver();

    ProfileQt *m_profile;
    std::unique_ptr<content::ResourceContext> m_resourceContext;
    scoped_refptr<ProtocolHandlerRegistry::IOThreadDelegate>
            m_protocolHandlerRegistryIOThreadDelegate;
    base::WeakPtr<ProfileIODataQt> m_weakPtr;
    scoped_refptr<CookieMonsterDelegateQt> m_cookieDelegate;
    content::URLRequestInterceptorScopedVector m_requestInterceptors;
    content::ProtocolHandlerMap m_protocolHandlers;
    mojo::InterfacePtrInfo<proxy_resolver::mojom::ProxyResolverFactory> m_proxyResolverFactoryInterface;
    QAtomicPointer<net::ProxyConfigService> m_proxyConfigService;
    QPointer<ProfileAdapter> m_profileAdapter; // never dereferenced in IO thread and it is passed by qpointer
    ProfileAdapter::PersistentCookiesPolicy m_persistentCookiesPolicy;
    std::unique_ptr<ProxyConfigMonitor> m_proxyConfigMonitor;

#if QT_CONFIG(ssl)
    ClientCertificateStoreData *m_clientCertificateStoreData;
#endif
    QString m_cookiesPath;
    QString m_httpAcceptLanguage;
    QString m_httpUserAgent;
    ProfileAdapter::HttpCacheType m_httpCacheType;
    QString m_httpCachePath;
    QList<QByteArray> m_customUrlSchemes;
    QList<QByteArray> m_installedCustomSchemes;
    QWebEngineUrlRequestInterceptor* m_requestInterceptor = nullptr;
    network::CookieSettings m_cookieSettings;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex m_mutex{QMutex::Recursive};
    using QRecursiveMutex = QMutex;
#else
    QRecursiveMutex m_mutex;
#endif
    int m_httpCacheMaxSize = 0;
    bool m_initialized = false;
    bool m_updateAllStorage = false;
    bool m_updateJobFactory = false;
    bool m_ignoreCertificateErrors = false;
    bool m_useForGlobalCertificateVerification = false;
    bool m_hasPageInterceptors = false;
    BrowsingDataRemoverObserverQt m_removerObserver;
    base::WeakPtrFactory<ProfileIODataQt> m_weakPtrFactory; // this should be always the last member
    QString m_dataPath;
    bool m_pendingStorageRequestGeneration = false;
    volatile bool m_isInterceptorDeprecated = false; // Remove for Qt6
    DISALLOW_COPY_AND_ASSIGN(ProfileIODataQt);

    friend class BrowsingDataRemoverObserverQt;
};
} // namespace QtWebEngineCore

#endif // PROFILE_IO_DATA_QT_H
