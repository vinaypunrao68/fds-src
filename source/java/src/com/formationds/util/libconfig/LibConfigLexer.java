// $ANTLR 3.5.1 /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g 2014-05-08 11:14:06

    package com.formationds.util.libconfig;


import org.antlr.runtime.*;

@SuppressWarnings("all")
public class LibConfigLexer extends Lexer {
	public static final int EOF=-1;
	public static final int T__17=17;
	public static final int T__18=18;
	public static final int T__19=19;
	public static final int BOOLEAN=4;
	public static final int COMMENT=5;
	public static final int EQUALS=6;
	public static final int ESC_SEQ=7;
	public static final int EXPONENT=8;
	public static final int FLOAT=9;
	public static final int HEX_DIGIT=10;
	public static final int ID=11;
	public static final int INT=12;
	public static final int OCTAL_ESC=13;
	public static final int STRING=14;
	public static final int UNICODE_ESC=15;
	public static final int WS=16;

	// delegates
	// delegators
	public Lexer[] getDelegates() {
		return new Lexer[] {};
	}

	public LibConfigLexer() {} 
	public LibConfigLexer(CharStream input) {
		this(input, new RecognizerSharedState());
	}
	public LibConfigLexer(CharStream input, RecognizerSharedState state) {
		super(input,state);
	}
	@Override public String getGrammarFileName() { return "/home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g"; }

	// $ANTLR start "T__17"
	public final void mT__17() throws RecognitionException {
		try {
			int _type = T__17;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:6:7: ( ':' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:6:9: ':'
			{
			match(':'); 
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "T__17"

	// $ANTLR start "T__18"
	public final void mT__18() throws RecognitionException {
		try {
			int _type = T__18;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:7:7: ( '{' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:7:9: '{'
			{
			match('{'); 
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "T__18"

	// $ANTLR start "T__19"
	public final void mT__19() throws RecognitionException {
		try {
			int _type = T__19;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:8:7: ( '}' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:8:9: '}'
			{
			match('}'); 
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "T__19"

	// $ANTLR start "EQUALS"
	public final void mEQUALS() throws RecognitionException {
		try {
			int _type = EQUALS;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:19:9: ( '=' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:19:11: '='
			{
			match('='); 
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "EQUALS"

	// $ANTLR start "BOOLEAN"
	public final void mBOOLEAN() throws RecognitionException {
		try {
			int _type = BOOLEAN;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:21:9: ( 'true' | 'false' )
			int alt1=2;
			int LA1_0 = input.LA(1);
			if ( (LA1_0=='t') ) {
				alt1=1;
			}
			else if ( (LA1_0=='f') ) {
				alt1=2;
			}

			else {
				NoViableAltException nvae =
					new NoViableAltException("", 1, 0, input);
				throw nvae;
			}

			switch (alt1) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:21:12: 'true'
					{
					match("true"); 

					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:21:21: 'false'
					{
					match("false"); 

					}
					break;

			}
			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "BOOLEAN"

	// $ANTLR start "ID"
	public final void mID() throws RecognitionException {
		try {
			int _type = ID;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:23:5: ( ( 'a' .. 'z' | 'A' .. 'Z' | '_' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' )* )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:23:7: ( 'a' .. 'z' | 'A' .. 'Z' | '_' ) ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' )*
			{
			if ( (input.LA(1) >= 'A' && input.LA(1) <= 'Z')||input.LA(1)=='_'||(input.LA(1) >= 'a' && input.LA(1) <= 'z') ) {
				input.consume();
			}
			else {
				MismatchedSetException mse = new MismatchedSetException(null,input);
				recover(mse);
				throw mse;
			}
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:23:31: ( 'a' .. 'z' | 'A' .. 'Z' | '0' .. '9' | '_' )*
			loop2:
			while (true) {
				int alt2=2;
				int LA2_0 = input.LA(1);
				if ( ((LA2_0 >= '0' && LA2_0 <= '9')||(LA2_0 >= 'A' && LA2_0 <= 'Z')||LA2_0=='_'||(LA2_0 >= 'a' && LA2_0 <= 'z')) ) {
					alt2=1;
				}

				switch (alt2) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
					{
					if ( (input.LA(1) >= '0' && input.LA(1) <= '9')||(input.LA(1) >= 'A' && input.LA(1) <= 'Z')||input.LA(1)=='_'||(input.LA(1) >= 'a' && input.LA(1) <= 'z') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

				default :
					break loop2;
				}
			}

			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "ID"

	// $ANTLR start "INT"
	public final void mINT() throws RecognitionException {
		try {
			int _type = INT;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:26:5: ( ( '0' .. '9' )+ )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:26:7: ( '0' .. '9' )+
			{
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:26:7: ( '0' .. '9' )+
			int cnt3=0;
			loop3:
			while (true) {
				int alt3=2;
				int LA3_0 = input.LA(1);
				if ( ((LA3_0 >= '0' && LA3_0 <= '9')) ) {
					alt3=1;
				}

				switch (alt3) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
					{
					if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

				default :
					if ( cnt3 >= 1 ) break loop3;
					EarlyExitException eee = new EarlyExitException(3, input);
					throw eee;
				}
				cnt3++;
			}

			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "INT"

	// $ANTLR start "FLOAT"
	public final void mFLOAT() throws RecognitionException {
		try {
			int _type = FLOAT;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:5: ( ( '0' .. '9' )+ '.' ( '0' .. '9' )* ( EXPONENT )? | '.' ( '0' .. '9' )+ ( EXPONENT )? | ( '0' .. '9' )+ EXPONENT )
			int alt10=3;
			alt10 = dfa10.predict(input);
			switch (alt10) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:9: ( '0' .. '9' )+ '.' ( '0' .. '9' )* ( EXPONENT )?
					{
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:9: ( '0' .. '9' )+
					int cnt4=0;
					loop4:
					while (true) {
						int alt4=2;
						int LA4_0 = input.LA(1);
						if ( ((LA4_0 >= '0' && LA4_0 <= '9')) ) {
							alt4=1;
						}

						switch (alt4) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
							{
							if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
								input.consume();
							}
							else {
								MismatchedSetException mse = new MismatchedSetException(null,input);
								recover(mse);
								throw mse;
							}
							}
							break;

						default :
							if ( cnt4 >= 1 ) break loop4;
							EarlyExitException eee = new EarlyExitException(4, input);
							throw eee;
						}
						cnt4++;
					}

					match('.'); 
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:25: ( '0' .. '9' )*
					loop5:
					while (true) {
						int alt5=2;
						int LA5_0 = input.LA(1);
						if ( ((LA5_0 >= '0' && LA5_0 <= '9')) ) {
							alt5=1;
						}

						switch (alt5) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
							{
							if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
								input.consume();
							}
							else {
								MismatchedSetException mse = new MismatchedSetException(null,input);
								recover(mse);
								throw mse;
							}
							}
							break;

						default :
							break loop5;
						}
					}

					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:37: ( EXPONENT )?
					int alt6=2;
					int LA6_0 = input.LA(1);
					if ( (LA6_0=='E'||LA6_0=='e') ) {
						alt6=1;
					}
					switch (alt6) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:30:37: EXPONENT
							{
							mEXPONENT(); 

							}
							break;

					}

					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:31:9: '.' ( '0' .. '9' )+ ( EXPONENT )?
					{
					match('.'); 
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:31:13: ( '0' .. '9' )+
					int cnt7=0;
					loop7:
					while (true) {
						int alt7=2;
						int LA7_0 = input.LA(1);
						if ( ((LA7_0 >= '0' && LA7_0 <= '9')) ) {
							alt7=1;
						}

						switch (alt7) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
							{
							if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
								input.consume();
							}
							else {
								MismatchedSetException mse = new MismatchedSetException(null,input);
								recover(mse);
								throw mse;
							}
							}
							break;

						default :
							if ( cnt7 >= 1 ) break loop7;
							EarlyExitException eee = new EarlyExitException(7, input);
							throw eee;
						}
						cnt7++;
					}

					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:31:25: ( EXPONENT )?
					int alt8=2;
					int LA8_0 = input.LA(1);
					if ( (LA8_0=='E'||LA8_0=='e') ) {
						alt8=1;
					}
					switch (alt8) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:31:25: EXPONENT
							{
							mEXPONENT(); 

							}
							break;

					}

					}
					break;
				case 3 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:32:9: ( '0' .. '9' )+ EXPONENT
					{
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:32:9: ( '0' .. '9' )+
					int cnt9=0;
					loop9:
					while (true) {
						int alt9=2;
						int LA9_0 = input.LA(1);
						if ( ((LA9_0 >= '0' && LA9_0 <= '9')) ) {
							alt9=1;
						}

						switch (alt9) {
						case 1 :
							// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
							{
							if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
								input.consume();
							}
							else {
								MismatchedSetException mse = new MismatchedSetException(null,input);
								recover(mse);
								throw mse;
							}
							}
							break;

						default :
							if ( cnt9 >= 1 ) break loop9;
							EarlyExitException eee = new EarlyExitException(9, input);
							throw eee;
						}
						cnt9++;
					}

					mEXPONENT(); 

					}
					break;

			}
			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "FLOAT"

	// $ANTLR start "COMMENT"
	public final void mCOMMENT() throws RecognitionException {
		try {
			int _type = COMMENT;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:36:5: ( '/*' ( options {greedy=false; } : . )* '*/' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:36:9: '/*' ( options {greedy=false; } : . )* '*/'
			{
			match("/*"); 

			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:36:14: ( options {greedy=false; } : . )*
			loop11:
			while (true) {
				int alt11=2;
				int LA11_0 = input.LA(1);
				if ( (LA11_0=='*') ) {
					int LA11_1 = input.LA(2);
					if ( (LA11_1=='/') ) {
						alt11=2;
					}
					else if ( ((LA11_1 >= '\u0000' && LA11_1 <= '.')||(LA11_1 >= '0' && LA11_1 <= '\uFFFF')) ) {
						alt11=1;
					}

				}
				else if ( ((LA11_0 >= '\u0000' && LA11_0 <= ')')||(LA11_0 >= '+' && LA11_0 <= '\uFFFF')) ) {
					alt11=1;
				}

				switch (alt11) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:36:42: .
					{
					matchAny(); 
					}
					break;

				default :
					break loop11;
				}
			}

			match("*/"); 

			_channel=HIDDEN;
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "COMMENT"

	// $ANTLR start "WS"
	public final void mWS() throws RecognitionException {
		try {
			int _type = WS;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:39:5: ( ( ' ' | '\\t' | '\\r' | '\\n' ) )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:39:9: ( ' ' | '\\t' | '\\r' | '\\n' )
			{
			if ( (input.LA(1) >= '\t' && input.LA(1) <= '\n')||input.LA(1)=='\r'||input.LA(1)==' ' ) {
				input.consume();
			}
			else {
				MismatchedSetException mse = new MismatchedSetException(null,input);
				recover(mse);
				throw mse;
			}
			_channel=HIDDEN;
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "WS"

	// $ANTLR start "STRING"
	public final void mSTRING() throws RecognitionException {
		try {
			int _type = STRING;
			int _channel = DEFAULT_TOKEN_CHANNEL;
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:47:5: ( '\"' ( ESC_SEQ |~ ( '\\\\' | '\"' ) )* '\"' )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:47:8: '\"' ( ESC_SEQ |~ ( '\\\\' | '\"' ) )* '\"'
			{
			match('\"'); 
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:47:12: ( ESC_SEQ |~ ( '\\\\' | '\"' ) )*
			loop12:
			while (true) {
				int alt12=3;
				int LA12_0 = input.LA(1);
				if ( (LA12_0=='\\') ) {
					alt12=1;
				}
				else if ( ((LA12_0 >= '\u0000' && LA12_0 <= '!')||(LA12_0 >= '#' && LA12_0 <= '[')||(LA12_0 >= ']' && LA12_0 <= '\uFFFF')) ) {
					alt12=2;
				}

				switch (alt12) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:47:14: ESC_SEQ
					{
					mESC_SEQ(); 

					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:47:24: ~ ( '\\\\' | '\"' )
					{
					if ( (input.LA(1) >= '\u0000' && input.LA(1) <= '!')||(input.LA(1) >= '#' && input.LA(1) <= '[')||(input.LA(1) >= ']' && input.LA(1) <= '\uFFFF') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

				default :
					break loop12;
				}
			}

			match('\"'); 
			}

			state.type = _type;
			state.channel = _channel;
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "STRING"

	// $ANTLR start "EXPONENT"
	public final void mEXPONENT() throws RecognitionException {
		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:52:10: ( ( 'e' | 'E' ) ( '+' | '-' )? ( '0' .. '9' )+ )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:52:12: ( 'e' | 'E' ) ( '+' | '-' )? ( '0' .. '9' )+
			{
			if ( input.LA(1)=='E'||input.LA(1)=='e' ) {
				input.consume();
			}
			else {
				MismatchedSetException mse = new MismatchedSetException(null,input);
				recover(mse);
				throw mse;
			}
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:52:22: ( '+' | '-' )?
			int alt13=2;
			int LA13_0 = input.LA(1);
			if ( (LA13_0=='+'||LA13_0=='-') ) {
				alt13=1;
			}
			switch (alt13) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
					{
					if ( input.LA(1)=='+'||input.LA(1)=='-' ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

			}

			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:52:33: ( '0' .. '9' )+
			int cnt14=0;
			loop14:
			while (true) {
				int alt14=2;
				int LA14_0 = input.LA(1);
				if ( ((LA14_0 >= '0' && LA14_0 <= '9')) ) {
					alt14=1;
				}

				switch (alt14) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
					{
					if ( (input.LA(1) >= '0' && input.LA(1) <= '9') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

				default :
					if ( cnt14 >= 1 ) break loop14;
					EarlyExitException eee = new EarlyExitException(14, input);
					throw eee;
				}
				cnt14++;
			}

			}

		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "EXPONENT"

	// $ANTLR start "HEX_DIGIT"
	public final void mHEX_DIGIT() throws RecognitionException {
		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:55:11: ( ( '0' .. '9' | 'a' .. 'f' | 'A' .. 'F' ) )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:
			{
			if ( (input.LA(1) >= '0' && input.LA(1) <= '9')||(input.LA(1) >= 'A' && input.LA(1) <= 'F')||(input.LA(1) >= 'a' && input.LA(1) <= 'f') ) {
				input.consume();
			}
			else {
				MismatchedSetException mse = new MismatchedSetException(null,input);
				recover(mse);
				throw mse;
			}
			}

		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "HEX_DIGIT"

	// $ANTLR start "ESC_SEQ"
	public final void mESC_SEQ() throws RecognitionException {
		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:59:5: ( '\\\\' ( 'b' | 't' | 'n' | 'f' | 'r' | '\\\"' | '\\'' | '\\\\' ) | UNICODE_ESC | OCTAL_ESC )
			int alt15=3;
			int LA15_0 = input.LA(1);
			if ( (LA15_0=='\\') ) {
				switch ( input.LA(2) ) {
				case '\"':
				case '\'':
				case '\\':
				case 'b':
				case 'f':
				case 'n':
				case 'r':
				case 't':
					{
					alt15=1;
					}
					break;
				case 'u':
					{
					alt15=2;
					}
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					{
					alt15=3;
					}
					break;
				default:
					int nvaeMark = input.mark();
					try {
						input.consume();
						NoViableAltException nvae =
							new NoViableAltException("", 15, 1, input);
						throw nvae;
					} finally {
						input.rewind(nvaeMark);
					}
				}
			}

			else {
				NoViableAltException nvae =
					new NoViableAltException("", 15, 0, input);
				throw nvae;
			}

			switch (alt15) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:59:9: '\\\\' ( 'b' | 't' | 'n' | 'f' | 'r' | '\\\"' | '\\'' | '\\\\' )
					{
					match('\\'); 
					if ( input.LA(1)=='\"'||input.LA(1)=='\''||input.LA(1)=='\\'||input.LA(1)=='b'||input.LA(1)=='f'||input.LA(1)=='n'||input.LA(1)=='r'||input.LA(1)=='t' ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:60:9: UNICODE_ESC
					{
					mUNICODE_ESC(); 

					}
					break;
				case 3 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:61:9: OCTAL_ESC
					{
					mOCTAL_ESC(); 

					}
					break;

			}
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "ESC_SEQ"

	// $ANTLR start "OCTAL_ESC"
	public final void mOCTAL_ESC() throws RecognitionException {
		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:66:5: ( '\\\\' ( '0' .. '3' ) ( '0' .. '7' ) ( '0' .. '7' ) | '\\\\' ( '0' .. '7' ) ( '0' .. '7' ) | '\\\\' ( '0' .. '7' ) )
			int alt16=3;
			int LA16_0 = input.LA(1);
			if ( (LA16_0=='\\') ) {
				int LA16_1 = input.LA(2);
				if ( ((LA16_1 >= '0' && LA16_1 <= '3')) ) {
					int LA16_2 = input.LA(3);
					if ( ((LA16_2 >= '0' && LA16_2 <= '7')) ) {
						int LA16_4 = input.LA(4);
						if ( ((LA16_4 >= '0' && LA16_4 <= '7')) ) {
							alt16=1;
						}

						else {
							alt16=2;
						}

					}

					else {
						alt16=3;
					}

				}
				else if ( ((LA16_1 >= '4' && LA16_1 <= '7')) ) {
					int LA16_3 = input.LA(3);
					if ( ((LA16_3 >= '0' && LA16_3 <= '7')) ) {
						alt16=2;
					}

					else {
						alt16=3;
					}

				}

				else {
					int nvaeMark = input.mark();
					try {
						input.consume();
						NoViableAltException nvae =
							new NoViableAltException("", 16, 1, input);
						throw nvae;
					} finally {
						input.rewind(nvaeMark);
					}
				}

			}

			else {
				NoViableAltException nvae =
					new NoViableAltException("", 16, 0, input);
				throw nvae;
			}

			switch (alt16) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:66:9: '\\\\' ( '0' .. '3' ) ( '0' .. '7' ) ( '0' .. '7' )
					{
					match('\\'); 
					if ( (input.LA(1) >= '0' && input.LA(1) <= '3') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					if ( (input.LA(1) >= '0' && input.LA(1) <= '7') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					if ( (input.LA(1) >= '0' && input.LA(1) <= '7') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:67:9: '\\\\' ( '0' .. '7' ) ( '0' .. '7' )
					{
					match('\\'); 
					if ( (input.LA(1) >= '0' && input.LA(1) <= '7') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					if ( (input.LA(1) >= '0' && input.LA(1) <= '7') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;
				case 3 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:68:9: '\\\\' ( '0' .. '7' )
					{
					match('\\'); 
					if ( (input.LA(1) >= '0' && input.LA(1) <= '7') ) {
						input.consume();
					}
					else {
						MismatchedSetException mse = new MismatchedSetException(null,input);
						recover(mse);
						throw mse;
					}
					}
					break;

			}
		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "OCTAL_ESC"

	// $ANTLR start "UNICODE_ESC"
	public final void mUNICODE_ESC() throws RecognitionException {
		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:73:5: ( '\\\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:73:9: '\\\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
			{
			match('\\'); 
			match('u'); 
			mHEX_DIGIT(); 

			mHEX_DIGIT(); 

			mHEX_DIGIT(); 

			mHEX_DIGIT(); 

			}

		}
		finally {
			// do for sure before leaving
		}
	}
	// $ANTLR end "UNICODE_ESC"

	@Override
	public void mTokens() throws RecognitionException {
		// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:8: ( T__17 | T__18 | T__19 | EQUALS | BOOLEAN | ID | INT | FLOAT | COMMENT | WS | STRING )
		int alt17=11;
		alt17 = dfa17.predict(input);
		switch (alt17) {
			case 1 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:10: T__17
				{
				mT__17(); 

				}
				break;
			case 2 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:16: T__18
				{
				mT__18(); 

				}
				break;
			case 3 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:22: T__19
				{
				mT__19(); 

				}
				break;
			case 4 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:28: EQUALS
				{
				mEQUALS(); 

				}
				break;
			case 5 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:35: BOOLEAN
				{
				mBOOLEAN(); 

				}
				break;
			case 6 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:43: ID
				{
				mID(); 

				}
				break;
			case 7 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:46: INT
				{
				mINT(); 

				}
				break;
			case 8 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:50: FLOAT
				{
				mFLOAT(); 

				}
				break;
			case 9 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:56: COMMENT
				{
				mCOMMENT(); 

				}
				break;
			case 10 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:64: WS
				{
				mWS(); 

				}
				break;
			case 11 :
				// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:1:67: STRING
				{
				mSTRING(); 

				}
				break;

		}
	}


	protected DFA10 dfa10 = new DFA10(this);
	protected DFA17 dfa17 = new DFA17(this);
	static final String DFA10_eotS =
		"\5\uffff";
	static final String DFA10_eofS =
		"\5\uffff";
	static final String DFA10_minS =
		"\2\56\3\uffff";
	static final String DFA10_maxS =
		"\1\71\1\145\3\uffff";
	static final String DFA10_acceptS =
		"\2\uffff\1\2\1\1\1\3";
	static final String DFA10_specialS =
		"\5\uffff}>";
	static final String[] DFA10_transitionS = {
			"\1\2\1\uffff\12\1",
			"\1\3\1\uffff\12\1\13\uffff\1\4\37\uffff\1\4",
			"",
			"",
			""
	};

	static final short[] DFA10_eot = DFA.unpackEncodedString(DFA10_eotS);
	static final short[] DFA10_eof = DFA.unpackEncodedString(DFA10_eofS);
	static final char[] DFA10_min = DFA.unpackEncodedStringToUnsignedChars(DFA10_minS);
	static final char[] DFA10_max = DFA.unpackEncodedStringToUnsignedChars(DFA10_maxS);
	static final short[] DFA10_accept = DFA.unpackEncodedString(DFA10_acceptS);
	static final short[] DFA10_special = DFA.unpackEncodedString(DFA10_specialS);
	static final short[][] DFA10_transition;

	static {
		int numStates = DFA10_transitionS.length;
		DFA10_transition = new short[numStates][];
		for (int i=0; i<numStates; i++) {
			DFA10_transition[i] = DFA.unpackEncodedString(DFA10_transitionS[i]);
		}
	}

	protected class DFA10 extends DFA {

		public DFA10(BaseRecognizer recognizer) {
			this.recognizer = recognizer;
			this.decisionNumber = 10;
			this.eot = DFA10_eot;
			this.eof = DFA10_eof;
			this.min = DFA10_min;
			this.max = DFA10_max;
			this.accept = DFA10_accept;
			this.special = DFA10_special;
			this.transition = DFA10_transition;
		}
		@Override
		public String getDescription() {
			return "29:1: FLOAT : ( ( '0' .. '9' )+ '.' ( '0' .. '9' )* ( EXPONENT )? | '.' ( '0' .. '9' )+ ( EXPONENT )? | ( '0' .. '9' )+ EXPONENT );";
		}
	}

	static final String DFA17_eotS =
		"\5\uffff\2\7\1\uffff\1\17\4\uffff\2\7\1\uffff\2\7\1\24\1\7\1\uffff\1\24";
	static final String DFA17_eofS =
		"\26\uffff";
	static final String DFA17_minS =
		"\1\11\4\uffff\1\162\1\141\1\uffff\1\56\4\uffff\1\165\1\154\1\uffff\1\145"+
		"\1\163\1\60\1\145\1\uffff\1\60";
	static final String DFA17_maxS =
		"\1\175\4\uffff\1\162\1\141\1\uffff\1\145\4\uffff\1\165\1\154\1\uffff\1"+
		"\145\1\163\1\172\1\145\1\uffff\1\172";
	static final String DFA17_acceptS =
		"\1\uffff\1\1\1\2\1\3\1\4\2\uffff\1\6\1\uffff\1\10\1\11\1\12\1\13\2\uffff"+
		"\1\7\4\uffff\1\5\1\uffff";
	static final String DFA17_specialS =
		"\26\uffff}>";
	static final String[] DFA17_transitionS = {
			"\2\13\2\uffff\1\13\22\uffff\1\13\1\uffff\1\14\13\uffff\1\11\1\12\12\10"+
			"\1\1\2\uffff\1\4\3\uffff\32\7\4\uffff\1\7\1\uffff\5\7\1\6\15\7\1\5\6"+
			"\7\1\2\1\uffff\1\3",
			"",
			"",
			"",
			"",
			"\1\15",
			"\1\16",
			"",
			"\1\11\1\uffff\12\10\13\uffff\1\11\37\uffff\1\11",
			"",
			"",
			"",
			"",
			"\1\20",
			"\1\21",
			"",
			"\1\22",
			"\1\23",
			"\12\7\7\uffff\32\7\4\uffff\1\7\1\uffff\32\7",
			"\1\25",
			"",
			"\12\7\7\uffff\32\7\4\uffff\1\7\1\uffff\32\7"
	};

	static final short[] DFA17_eot = DFA.unpackEncodedString(DFA17_eotS);
	static final short[] DFA17_eof = DFA.unpackEncodedString(DFA17_eofS);
	static final char[] DFA17_min = DFA.unpackEncodedStringToUnsignedChars(DFA17_minS);
	static final char[] DFA17_max = DFA.unpackEncodedStringToUnsignedChars(DFA17_maxS);
	static final short[] DFA17_accept = DFA.unpackEncodedString(DFA17_acceptS);
	static final short[] DFA17_special = DFA.unpackEncodedString(DFA17_specialS);
	static final short[][] DFA17_transition;

	static {
		int numStates = DFA17_transitionS.length;
		DFA17_transition = new short[numStates][];
		for (int i=0; i<numStates; i++) {
			DFA17_transition[i] = DFA.unpackEncodedString(DFA17_transitionS[i]);
		}
	}

	protected class DFA17 extends DFA {

		public DFA17(BaseRecognizer recognizer) {
			this.recognizer = recognizer;
			this.decisionNumber = 17;
			this.eot = DFA17_eot;
			this.eof = DFA17_eof;
			this.min = DFA17_min;
			this.max = DFA17_max;
			this.accept = DFA17_accept;
			this.special = DFA17_special;
			this.transition = DFA17_transition;
		}
		@Override
		public String getDescription() {
			return "1:1: Tokens : ( T__17 | T__18 | T__19 | EQUALS | BOOLEAN | ID | INT | FLOAT | COMMENT | WS | STRING );";
		}
	}

}
