BEGIN {
D["PACKAGE_NAME"]=" \"LuminariMUD\""
D["PACKAGE_TARNAME"]=" \"luminarimud\""
D["PACKAGE_VERSION"]=" \"2.5002-beta\""
D["PACKAGE_STRING"]=" \"LuminariMUD 2.5002-beta\""
D["PACKAGE_BUGREPORT"]=" \"bugs@luminarimud.com\""
D["PACKAGE_URL"]=" \"\""
D["PACKAGE"]=" \"luminarimud\""
D["VERSION"]=" \"2.5002-beta\""
D["CIRCLE_UNIX"]=" 1"
D["CIRCLE_CRYPT"]=" 1"
D["HAVE_JSON_C"]=" 1"
D["HAVE_STDIO_H"]=" 1"
D["HAVE_STDLIB_H"]=" 1"
D["HAVE_STRING_H"]=" 1"
D["HAVE_INTTYPES_H"]=" 1"
D["HAVE_STDINT_H"]=" 1"
D["HAVE_STRINGS_H"]=" 1"
D["HAVE_SYS_STAT_H"]=" 1"
D["HAVE_SYS_TYPES_H"]=" 1"
D["HAVE_UNISTD_H"]=" 1"
D["STDC_HEADERS"]=" 1"
D["HAVE_SYS_WAIT_H"]=" 1"
D["HAVE_FCNTL_H"]=" 1"
D["HAVE_SYS_FCNTL_H"]=" 1"
D["HAVE_ERRNO_H"]=" 1"
D["HAVE_STRING_H"]=" 1"
D["HAVE_STRINGS_H"]=" 1"
D["HAVE_LIMITS_H"]=" 1"
D["HAVE_SYS_TIME_H"]=" 1"
D["HAVE_SYS_SELECT_H"]=" 1"
D["HAVE_SYS_TYPES_H"]=" 1"
D["HAVE_UNISTD_H"]=" 1"
D["HAVE_MEMORY_H"]=" 1"
D["HAVE_CRYPT_H"]=" 1"
D["HAVE_ASSERT_H"]=" 1"
D["HAVE_ARPA_TELNET_H"]=" 1"
D["HAVE_ARPA_INET_H"]=" 1"
D["HAVE_SYS_STAT_H"]=" 1"
D["HAVE_SYS_SOCKET_H"]=" 1"
D["HAVE_SYS_RESOURCE_H"]=" 1"
D["HAVE_NETINET_IN_H"]=" 1"
D["HAVE_NETDB_H"]=" 1"
D["HAVE_SIGNAL_H"]=" 1"
D["HAVE_SYS_UIO_H"]=" 1"
D["HAVE_MCHECK_H"]=" 1"
D["HAVE_TIME_H"]=" 1"
D["HAVE_SYS_TIME_H"]=" 1"
D["HAVE_DECL_STRUCT_TIMEVAL"]=" 0"
D["HAVE_STRUCT_IN_ADDR"]=" 1"
D["HAVE_VPRINTF"]=" 1"
D["HAVE_GETTIMEOFDAY"]=" 1"
D["HAVE_SELECT"]=" 1"
D["HAVE_SNPRINTF"]=" 1"
D["HAVE_STRCASECMP"]=" 1"
D["HAVE_STRDUP"]=" 1"
D["HAVE_STRERROR"]=" 1"
D["HAVE_STRLCPY"]=" 1"
D["HAVE_STRNCASECMP"]=" 1"
D["HAVE_STRSTR"]=" 1"
D["HAVE_VSNPRINTF"]=" 1"
D["HAVE_INET_ADDR"]=" 1"
D["HAVE_INET_ATON"]=" 1"
  for (key in D) D_is_set[key] = 1
  FS = ""
}
/^[\t ]*#[\t ]*(define|undef)[\t ]+[_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]*([\t (]|$)/ {
  line = $ 0
  split(line, arg, " ")
  if (arg[1] == "#") {
    defundef = arg[2]
    mac1 = arg[3]
  } else {
    defundef = substr(arg[1], 2)
    mac1 = arg[2]
  }
  split(mac1, mac2, "(") #)
  macro = mac2[1]
  prefix = substr(line, 1, index(line, defundef) - 1)
  if (D_is_set[macro]) {
    # Preserve the white space surrounding the "#".
    print prefix "define", macro P[macro] D[macro]
    next
  } else {
    # Replace #undef with comments.  This is necessary, for example,
    # in the case of _POSIX_SOURCE, which is predefined and required
    # on some systems where configure will not decide to define it.
    if (defundef == "undef") {
      print "/*", prefix defundef, macro, "*/"
      next
    }
  }
}
{ print }
