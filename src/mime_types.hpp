#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <unordered_map>

/** \brief Provide MIME type lookups based on a CUPS-style database file.
 * \remarks The CUPS MIME database is typically located at /etc/mime.types.
 * Its syntax is very simple; see man 5 mime.types. This database just assigns
 * MIME types based on filename suffixes, not looking at magic numbers or
 * other heuristics, as does libmagic. */
namespace mime_types
{
  class _mime_types
  {
    static std::unordered_map<std::string, std::string> types;
    static char const *fallback_type;
  public:
    /** \brief Parse the MIME database into a constant-time lookup table.
     * \param mimedb path to CUPS MIME database
     * \param MIME type to use if we can't find anything better
     * \remarks The intent is to initialize the static _mime_types object
     * at the beginning of the process and use that for all lookups. */
    void init(char const *mimedb="/etc/mime.types",
	      char const *fallback_type="application/octet-stream");
    /** \brief Look up the MIME type of a file based on suffix.
     * \param filename the file name
     * \return MIME type string */
    char const *operator()(std::string const &filename);
  };
  static _mime_types lookup;
};

#endif // MIME_TYPES_HPP
