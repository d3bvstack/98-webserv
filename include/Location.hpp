/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbarba-v <dbarba-v@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 20:07:04 by dbarba-v          #+#    #+#             */
/*   Updated: 2026/06/26 12:54:33 by dbarba-v         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <utility>

class Vhost;

class Location
{
    private:
        bool        _autoindex;
        bool        _autoindex_set;
        std::string _path;
        std::string _root;
        std::string _upload_store;
        uint64_t   _max_body_size;
        bool        _max_body_size_set;
        std::vector<std::string> _defaults;
        std::pair<uint16_t, std::string>  _return;
        std::vector<std::string> _methods;

    public:
        Location(const Vhost& currentVhost);
        ~Location();

        bool isAutoindexSet() const                                 { return _autoindex_set; }
        bool isPathSet() const                                      { return !_path.empty(); }
        bool isRootSet() const                                      { return !_root.empty(); }
        bool isUploadStoreSet() const                               { return !_upload_store.empty(); }
        bool isDefaultsSet() const                                  { return !_defaults.empty(); }
        bool isReturnSet() const                                    { return _return.first != 0; }
        bool isMethodsSet() const                                   { return !_methods.empty(); }
        bool isMaxBodySizeSet() const                               { return _max_body_size_set; }

        bool getAutoindex() const                                   { return _autoindex; }
        const std::string& getPath() const                          { return _path; }
        const std::string& getRoot() const                          { return _root; }
        const std::string& getUploadStore() const                   { return _upload_store; }
        const std::vector<std::string>& getDefaults() const         { return _defaults; }
        const std::pair<uint16_t, std::string>& getReturn() const   { return _return; }
        const std::vector<std::string>& getMethods() const          { return _methods; }
        uint64_t getMaxBodySize() const                             { return _max_body_size; }

        void setAutoindex(const std::string& value);
        void setPath(const std::string& path);
        void setRoot(const std::string& root);
        void setUploadStore(const std::string& upload_store);
        void setMaxBodySize(const std::string& value);
        void addDefault(const std::string& default_file);
        void setReturn(const std::string& value);
        void addMethod(const std::string& method);

        void verify() const;
        void debugLocation() const;

};
