TEST_DEPS= 							-package Qtest2lib

OCAML_FLAGS= 						$(INCLUDES)
OCAML_TEST_FLAGS=				$(OCAML_FLAGS) $(TEST_DEPS) -I . -w +26 -linkpkg

OCAML= 									ocamlc $(OCAML_FLAGS)
OCAML_TEST=							ocamlfind ocamlc $(OCAML_TEST_FLAGS)

TEST_CASES=							$(shell ls | grep .ml | sed 's/\(.*\).ml/_test\/\1tests/')
LINK_ORDER= 						$(shell cat .depend | grep .cmo | tail -r | sed 's/ :.*//' | tr '\n' ' ')

clean:; rm -rf .depend *.o *.cm[io] _test
watch:; fswatch -l 0.1 -o *.ml | xargs -n1 -I{} make -s test
test:$(TEST_CASES); sh $(TOOLS_DIR)/run_tests.sh;

_test/%tests.ml: %.ml; mkdir -p _test; qtest -o $@ extract $<
_test/%tests: _test/%tests.ml %.cmo; $(OCAML_TEST) -o $@ $(LINK_ORDER) $<

%.cmo %.cmi: %.ml; $(OCAML) -c $<

.depend:; ocamldep *.ml > .depend
-include .depend

.PHONY: watch clean test
