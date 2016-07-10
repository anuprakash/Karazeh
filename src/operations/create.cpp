/**
 * karazeh -- the library for patching software
 *
 * Copyright (C) 2011-2016 by Ahmad Amireh <ahmad@amireh.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "karazeh/operations/create.hpp"

namespace kzh {
  namespace fs = boost::filesystem;

  create_operation::create_operation(
    config_t const& config,
    file_manager const& file_manager,
    downloader const& downloader,
    release_manifest& rm
  )
  : operation(config, file_manager, downloader, rm),
    logger("op_create"),
    created_directory_(false),
    created_(false),
    marked_for_deletion_(false),
    is_executable(false)
  {
  }

  create_operation::~create_operation() {
  }

  void create_operation::marked_for_deletion() {
    marked_for_deletion_ = true;
  }

  STAGE_RC create_operation::stage() {
    cache_path_ = path_t(config_.cache_path / rm_.checksum / dst_path);
    cache_dir_ = cache_path_.parent_path();
    dst_dir_ = (config_.root_path / dst_path).parent_path();

    if (config_.verbose) {
      indent();

      debug() << "Caching path: "<< cache_path_;
      debug() << "Caching dir: "<< cache_dir_;
      debug() << "Dest dir: "<< dst_dir_;
      debug() << "Dest path: " << dst_path;

      deindent();
    }

    // Prepare our staging directory
    if (!file_manager_.ensure_directory(cache_dir_)) {
      error() << "Unable to create caching directory: " << cache_dir_;
      return STAGE_UNAUTHORIZED;
    }

    // Prepare our destination directory, if necessary
    if (!file_manager_.exists(dst_dir_)) {
      if (!file_manager_.create_directory(dst_dir_)) {
        error() << "Unable to create destination dir: " << dst_dir_;
        return STAGE_UNAUTHORIZED;
      }

      created_directory_ = true;
    }

    // Make sure the destination is free
    if (file_manager_.is_readable(config_.root_path / dst_path) && !marked_for_deletion_) {
      error() << "Destination is occupied: " << dst_path;

      return STAGE_FILE_EXISTS;
    }

    // Can we write to the destination?
    if (!file_manager_.is_writable(config_.root_path / dst_path)) {
      error() << "Destination isn't writable: " << config_.root_path / dst_path;
      return STAGE_UNAUTHORIZED;
    }

    // Can we write to the staging destination?
    if (!file_manager_.is_writable(cache_path_)) {
      error() << "The cache isn't writable: " << cache_path_;
      return STAGE_UNAUTHORIZED;
    }

    if (!downloader_.fetch(src_uri, cache_path_, src_checksum, src_size)) {
      throw invalid_resource(src_uri);
    }

    return STAGE_OK;
  }

  STAGE_RC create_operation::deploy() {
    using fs::rename;

    // Make sure the destination is free
    if (file_manager_.is_readable(config_.root_path / dst_path)) {
      error() << "Destination is occupied: " << dst_path;
      return STAGE_FILE_EXISTS;
    }

    // Can we write to the destination?
    if (!file_manager_.is_writable(config_.root_path / dst_path)) {
      error() << "Destination isn't writable: " << config_.root_path /dst_path;
      return STAGE_UNAUTHORIZED;
    }

    // Can we write to the staging destination?
    if (!file_manager_.is_writable(cache_path_)) {
      error() << "Temp isn't writable: " << cache_path_;
      return STAGE_UNAUTHORIZED;
    }

    // Move the staged file to the destination
    info() << "Creating " << config_.root_path / dst_path;
    rename(cache_path_, config_.root_path / dst_path);
    created_ = true;

    // validate integrity
    std::ifstream fh((config_.root_path / dst_path).string().c_str());
    hasher::digest_rc rc = config_.hasher->hex_digest(fh);
    fh.close();

    if (rc != src_checksum) {
      error() << "Created file integrity mismatch: " << rc.digest << " vs " << src_checksum;
      return STAGE_FILE_INTEGRITY_MISMATCH;
    }

    if (is_executable) {
      file_manager_.make_executable(config_.root_path / dst_path);
    }

    return STAGE_OK;
  }

  void create_operation::rollback() {
    using fs::is_empty;
    using fs::remove;

    if (created_) {
      remove(config_.root_path / dst_path);
    }

    // If we were responsible for creating the directory of our dst_path
    // we need to remove it and all its ancestors if they're empty
    if (created_directory_) {
      path_t curr_dir = dst_dir_;
      while (is_empty(curr_dir)) {
        remove(curr_dir);
        curr_dir = curr_dir.parent_path();
      }
    }

    return;
  }

  string_t create_operation::tostring() {
    std::ostringstream s;
    s << "create from[" << this->src_uri << ']'
      << " to[" << this->dst_path << ']'
      << " checksum[" << this->src_checksum << ']'
      << " size[" << this->src_size << ']';
    return s.str();
  }
}