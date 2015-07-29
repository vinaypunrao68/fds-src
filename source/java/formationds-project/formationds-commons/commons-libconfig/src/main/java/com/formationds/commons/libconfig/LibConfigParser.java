// $ANTLR 3.5.1 /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g 2014-05-13 11:48:36

    package com.formationds.commons.libconfig;


import org.antlr.runtime.BitSet;
import org.antlr.runtime.MismatchedSetException;
import org.antlr.runtime.Parser;
import org.antlr.runtime.ParserRuleReturnScope;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.RecognizerSharedState;
import org.antlr.runtime.Token;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeAdaptor;
import org.antlr.runtime.tree.TreeAdaptor;


@SuppressWarnings("all")
public class LibConfigParser extends Parser {
	public static final String[] tokenNames = new String[] {
		"<invalid>", "<EOR>", "<DOWN>", "<UP>", "BOOLEAN", "COMMENT", "EQUALS", 
		"ESC_SEQ", "EXPONENT", "FLOAT", "HEX_DIGIT", "ID", "INT", "OCTAL_ESC", 
		"STRING", "UNICODE_ESC", "WS", "':'", "';'", "'{'", "'}'"
	};
	public static final int EOF=-1;
	public static final int T__17=17;
	public static final int T__18=18;
	public static final int T__19=19;
	public static final int T__20=20;
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
	public Parser[] getDelegates() {
		return new Parser[] {};
	}

	// delegators


	public LibConfigParser(TokenStream input) {
		this(input, new RecognizerSharedState());
	}
	public LibConfigParser(TokenStream input, RecognizerSharedState state) {
		super(input, state);
	}

	protected TreeAdaptor adaptor = new CommonTreeAdaptor();

	public void setTreeAdaptor(TreeAdaptor adaptor) {
		this.adaptor = adaptor;
	}
	public TreeAdaptor getTreeAdaptor() {
		return adaptor;
	}
	@Override public String[] getTokenNames() { return LibConfigParser.tokenNames; }
	@Override public String getGrammarFileName()
	{
		return "LibConfig.g";
	}


	public static class namespace_return extends ParserRuleReturnScope {
		Object tree;
		@Override
		public Object getTree() { return tree; }
	};


	// $ANTLR start "namespace"
	// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:1: namespace : ID ^ ':' ! '{' ! ( namespace | assignment )* '}' !;
	public final LibConfigParser.namespace_return namespace() throws RecognitionException {
		LibConfigParser.namespace_return retval = new LibConfigParser.namespace_return();
		retval.start = input.LT(1);

		Object root_0 = null;

		Token ID1=null;
		Token char_literal2=null;
		Token char_literal3=null;
		Token char_literal6=null;
		ParserRuleReturnScope namespace4 =null;
		ParserRuleReturnScope assignment5 =null;

		Object ID1_tree=null;
		Object char_literal2_tree=null;
		Object char_literal3_tree=null;
		Object char_literal6_tree=null;

		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:11: ( ID ^ ':' ! '{' ! ( namespace | assignment )* '}' !)
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:13: ID ^ ':' ! '{' ! ( namespace | assignment )* '}' !
			{
			root_0 = (Object)adaptor.nil();


			ID1=(Token)match(input,ID,FOLLOW_ID_in_namespace39); 
			ID1_tree = (Object)adaptor.create(ID1);
			root_0 = (Object)adaptor.becomeRoot(ID1_tree, root_0);

			char_literal2=(Token)match(input,17,FOLLOW_17_in_namespace42); 
			char_literal3=(Token)match(input,19,FOLLOW_19_in_namespace45); 
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:28: ( namespace | assignment )*
			loop1:
			while (true) {
				int alt1=3;
				int LA1_0 = input.LA(1);
				if ( (LA1_0==ID) ) {
					int LA1_2 = input.LA(2);
					if ( (LA1_2==17) ) {
						alt1=1;
					}
					else if ( (LA1_2==EQUALS) ) {
						alt1=2;
					}

				}

				switch (alt1) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:29: namespace
					{
					pushFollow(FOLLOW_namespace_in_namespace50);
					namespace4=namespace();
					state._fsp--;

					adaptor.addChild(root_0, namespace4.getTree());

					}
					break;
				case 2 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:15:41: assignment
					{
					pushFollow(FOLLOW_assignment_in_namespace54);
					assignment5=assignment();
					state._fsp--;

					adaptor.addChild(root_0, assignment5.getTree());

					}
					break;

				default :
					break loop1;
				}
			}

			char_literal6=(Token)match(input,20,FOLLOW_20_in_namespace59); 
			}

			retval.stop = input.LT(-1);

			retval.tree = (Object)adaptor.rulePostProcessing(root_0);
			adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

		}
		catch (RecognitionException re) {
			reportError(re);
			recover(input,re);
			retval.tree = (Object)adaptor.errorNode(input, retval.start, input.LT(-1), re);
		}
		finally {
			// do for sure before leaving
		}
		return retval;
	}
	// $ANTLR end "namespace"


	public static class assignment_return extends ParserRuleReturnScope {
		Object tree;
		@Override
		public Object getTree() { return tree; }
	};


	// $ANTLR start "assignment"
	// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:17:1: assignment : ID EQUALS ^ ( INT | FLOAT | STRING | BOOLEAN ) ( ';' )? ;
	public final LibConfigParser.assignment_return assignment() throws RecognitionException {
		LibConfigParser.assignment_return retval = new LibConfigParser.assignment_return();
		retval.start = input.LT(1);

		Object root_0 = null;

		Token ID7=null;
		Token EQUALS8=null;
		Token set9=null;
		Token char_literal10=null;

		Object ID7_tree=null;
		Object EQUALS8_tree=null;
		Object set9_tree=null;
		Object char_literal10_tree=null;

		try {
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:17:11: ( ID EQUALS ^ ( INT | FLOAT | STRING | BOOLEAN ) ( ';' )? )
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:17:14: ID EQUALS ^ ( INT | FLOAT | STRING | BOOLEAN ) ( ';' )?
			{
			root_0 = (Object)adaptor.nil();


			ID7=(Token)match(input,ID,FOLLOW_ID_in_assignment71); 
			ID7_tree = (Object)adaptor.create(ID7);
			adaptor.addChild(root_0, ID7_tree);

			EQUALS8=(Token)match(input,EQUALS,FOLLOW_EQUALS_in_assignment73); 
			EQUALS8_tree = (Object)adaptor.create(EQUALS8);
			root_0 = (Object)adaptor.becomeRoot(EQUALS8_tree, root_0);

			set9=input.LT(1);
			if ( input.LA(1)==BOOLEAN||input.LA(1)==FLOAT||input.LA(1)==INT||input.LA(1)==STRING ) {
				input.consume();
				adaptor.addChild(root_0, (Object)adaptor.create(set9));
				state.errorRecovery=false;
			}
			else {
				MismatchedSetException mse = new MismatchedSetException(null,input);
				throw mse;
			}
			// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:17:58: ( ';' )?
			int alt2=2;
			int LA2_0 = input.LA(1);
			if ( (LA2_0==18) ) {
				alt2=1;
			}
			switch (alt2) {
				case 1 :
					// /home/fabrice/fds-src/source/java/src/com/formationds/util/libconfig/LibConfig.g:17:59: ';'
					{
					char_literal10=(Token)match(input,18,FOLLOW_18_in_assignment93); 
					char_literal10_tree = (Object)adaptor.create(char_literal10);
					adaptor.addChild(root_0, char_literal10_tree);

					}
					break;

			}

			}

			retval.stop = input.LT(-1);

			retval.tree = (Object)adaptor.rulePostProcessing(root_0);
			adaptor.setTokenBoundaries(retval.tree, retval.start, retval.stop);

		}
		catch (RecognitionException re) {
			reportError(re);
			recover(input,re);
			retval.tree = (Object)adaptor.errorNode(input, retval.start, input.LT(-1), re);
		}
		finally {
			// do for sure before leaving
		}
		return retval;
	}
	// $ANTLR end "assignment"

	// Delegated rules



	public static final BitSet FOLLOW_ID_in_namespace39 = new BitSet(new long[]{0x0000000000020000L});
	public static final BitSet FOLLOW_17_in_namespace42 = new BitSet(new long[]{0x0000000000080000L});
	public static final BitSet FOLLOW_19_in_namespace45 = new BitSet(new long[]{0x0000000000100800L});
	public static final BitSet FOLLOW_namespace_in_namespace50 = new BitSet(new long[]{0x0000000000100800L});
	public static final BitSet FOLLOW_assignment_in_namespace54 = new BitSet(new long[]{0x0000000000100800L});
	public static final BitSet FOLLOW_20_in_namespace59 = new BitSet(new long[]{0x0000000000000002L});
	public static final BitSet FOLLOW_ID_in_assignment71 = new BitSet(new long[]{0x0000000000000040L});
	public static final BitSet FOLLOW_EQUALS_in_assignment73 = new BitSet(new long[]{0x0000000000005210L});
	public static final BitSet FOLLOW_set_in_assignment76 = new BitSet(new long[]{0x0000000000040002L});
	public static final BitSet FOLLOW_18_in_assignment93 = new BitSet(new long[]{0x0000000000000002L});
}
