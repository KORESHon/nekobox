


#include <nekobox/dataStore/ProfileFilter.hpp>
#include <set>


namespace Configs {

// --- Equality ---
bool ProfileFilterKey::operator==(const ProfileFilterKey &other) const noexcept
{
  const bool this_null = key == nullptr;
  const bool other_null = other.key == nullptr;
  if (this_null || other_null) {
    return this_null == other_null;
  }
  if (key->type != other.key->type ||
      key->serverAddress != other.key->serverAddress ||
      key->serverPort != other.key->serverPort ||
      key->name != other.key->name ||
      skip_compare_beans != other.skip_compare_beans) {
    return false;
  }
  // Avoid JsonStore::compare() — unsafe for ordering/equality containers.
  return true;
}

bool ProfileFilterKey::operator!=(const ProfileFilterKey &other) const noexcept
{
    return !(*this == other);
}

// --- Ordering (STRICT WEAK ORDERING) ---
bool ProfileFilterKey::operator<(const ProfileFilterKey &other) const noexcept
{
  const bool this_null = key == nullptr;
  const bool other_null = other.key == nullptr;
  if (this_null != other_null) {
    return this_null; // nullptr sorts before non-null
  }
  if (this_null) {
    return false;
  }

  if (key->type != other.key->type) {
    return key->type < other.key->type;
  }
  if (key->serverAddress != other.key->serverAddress) {
    return key->serverAddress < other.key->serverAddress;
  }
  if (key->serverPort != other.key->serverPort) {
    return key->serverPort < other.key->serverPort;
  }
  // Distinguish TLS vs port-hopping (and similar) profiles that share host:port
  // but have different names / hop settings. Do NOT call JsonStore::compare().
  if (key->name != other.key->name) {
    return key->name < other.key->name;
  }
  return false;
}

bool ProfileFilterKey::operator>(const ProfileFilterKey &other) const noexcept
{
    return other < *this;
}

bool ProfileFilterKey::operator<=(const ProfileFilterKey &other) const noexcept
{
    return !(other < *this);
}

bool ProfileFilterKey::operator>=(const ProfileFilterKey &other) const noexcept
{
    return !(*this < other);
}


// --- Helper factory ---
ProfileFilterKey ProfileFilter_ent_key(
    const std::shared_ptr<Configs::ProxyEntity> &ent,
    bool by_address)
{
    const bool useAddressOnly = by_address && ent->type != "custom";
    return ProfileFilterKey(ent, !useAddressOnly);
}

    ProfileFilterKey::ProfileFilterKey(const std::shared_ptr<Configs::ProxyEntity>& key,
                     bool unpack_bean) noexcept
    {
      this->key = key;
      this->skip_compare_beans = unpack_bean;
    }

void ProfileFilter::Uniq(const QList<std::shared_ptr<ProxyEntity>> &in,
                         QList<std::shared_ptr<ProxyEntity>> &out,
                         bool by_address, bool keep_last) {
  std::map<ProfileFilterKey, std::shared_ptr<ProxyEntity>> hashMap;

  for (const auto &ent : in) {
    ProfileFilterKey key = ProfileFilter_ent_key(ent, by_address);
    auto iter = hashMap.find(key);
    auto found = iter != hashMap.end();
    if (found && keep_last) {
      out.removeAll(iter->second);
      found = false;
    } 
    if (!found) {
      hashMap[key] = ent;
      out += ent;
    } 
  }
}

void ProfileFilter::Common(const QList<std::shared_ptr<ProxyEntity>> &src,
                           const QList<std::shared_ptr<ProxyEntity>> &dst,
                           QList<std::shared_ptr<ProxyEntity>> &outSrc,
                           QList<std::shared_ptr<ProxyEntity>> &outDst,
                           bool by_address) {
  std::map<ProfileFilterKey, std::shared_ptr<ProxyEntity>> map;

  for (const auto &ent : src) {
    map.insert({ProfileFilter_ent_key(ent, by_address), ent});
  }

  for (const auto &ent : dst) {
    const ProfileFilterKey key = ProfileFilter_ent_key(ent, by_address);
    auto iter = map.find(key);
    if (iter != map.end()) {
      outDst += ent;
      outSrc += iter->second;
    }
  }
}

void ProfileFilter::OnlyInSrc(const QList<std::shared_ptr<ProxyEntity>> &src,
                              const QList<std::shared_ptr<ProxyEntity>> &dst,
                              QList<std::shared_ptr<ProxyEntity>> &out,
                              bool by_address) {
  std::set<ProfileFilterKey> keys;

  for (const auto &ent : src) {
    auto key = ProfileFilter_ent_key(ent, by_address);
    if (!keys.contains(key)) {
      out += ent;
      keys.insert(key);
    }
  }
}

void ProfileFilter::OnlyInSrc_ByPointer(
    const QList<std::shared_ptr<ProxyEntity>> &src,
    const QList<std::shared_ptr<ProxyEntity>> &dst,
    QList<std::shared_ptr<ProxyEntity>> &out) {
  for (const auto &ent : src) {
    if (!dst.contains(ent))
      out += ent;
  }
}


void ProfileFilter::OnlyInSrc_ByIds(
    const QList<std::shared_ptr<ProxyEntity>> &src,
    const QList<std::shared_ptr<ProxyEntity>> &dst,
    QList<std::shared_ptr<ProxyEntity>> &out) {
  std::set<int> ids;
  for (const auto &ent : src) {
    int id;
    if (!ids.contains(id = ent->Id())) {
      out += ent;
      ids.insert(id);
    }
  }
}

} // namespace Configs

