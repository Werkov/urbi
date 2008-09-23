/// \file parser/uparser.hh

#ifndef PARSER_UPARSER_HH
# define PARSER_UPARSER_HH

# include <memory>
# include <string>

# include <parser/fwd.hh>
# include <parser/location.hh>

namespace parser
{

  /// Persistent parsing.
  ///
  /// The main point of this class is to allow parsing of a pseudo
  /// stream while maintaining a stack of locations.  It is actually a
  /// facade around a ParserImpl in order to avoid useless
  /// recompilations.
  class UParser
  {
  public:
    UParser();
    UParser(const UParser&);
    ~UParser();

    /// Parse the command from a buffer.
    /// \return yyparse's result (0 on success).
    parse_result_type parse(const std::string& code,
                            const yy::location& loc = yy::location());

    /// Parse a file.
    /// \return yyparse's result (0 on success).
    parse_result_type parse_file(const std::string& fn);

  private:
    std::auto_ptr<ParserImpl> pimpl_;
  };

}

#endif // !PARSER_UPARSER_HH
