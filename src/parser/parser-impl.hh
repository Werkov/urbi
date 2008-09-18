/// \file parser/parser-impl.hh

#ifndef PARSER_PARSER_IMPL_HH
# define PARSER_PARSER_IMPL_HH

# include <list>
# include <memory>
# include <string>

# include <parser/fwd.hh>
# include <parser/parse-result.hh>
# include <parser/utoken.hh>

namespace parser
{

  class ParserImpl
  {
  public:
    typedef yy::parser parser_type;
    typedef parser_type::token_type token_type;
    typedef parser_type::semantic_type semantic_type;
    typedef parser_type::location_type location_type;
    typedef parser_type::symbol_type symbol_type;

    ParserImpl();

    /// Parse the command from a buffer.
    parse_result_type parse(const std::string& code);

    /// Parse a file.
    parse_result_type parse_file(const std::string& fn);

  public:
    /// Declare an error at \a l about \a msg.
    void error(const location_type& l, const std::string& msg);

    /// Warn at \a l about \a msg.
    void warn(const location_type& l, const std::string& msg);

  private:
    // Give access to loc_.
    friend int parser_type::parse();
    friend YY_DECL;

    /// Run the parse.
    /// Store the result in \c result_.
    void parse_(std::istream& source);

    /// The current location.
    location_type loc_;

    /// A stack of locations to support //#push and //#pop.
    std::stack<yy::location> synclines_;

    /// The resut of the parsing.
    parse_result_type result_;

    /// Whether in debug mode.
    /// Enabled when the envvar YYDEBUG is defined (whatever the value).
    bool debug_;
  };

}

#endif // !PARSER_PARSER_IMPL_HH
