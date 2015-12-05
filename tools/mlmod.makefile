TEST_DEPS= 							-package Qtest2lib

OCAML_FLAGS= 						$(INCLUDES)
OCAML_TEST_FLAGS=				$(OCAML_FLAGS) $(TEST_DEPS) -I . -linkpkg

OCAML= 									ocamlc $(OCAML_FLAGS)
OCAML_TEST=							ocamlfind ocamlc $(OCAML_TEST_FLAGS)

LIST_TESTS=							bash $(TOOLS_DIR)/list_tests.sh
GET_LINK_ORDER= 				node $(TOOLS_DIR)/ml_link_order
RUN_TESTS= 							bash $(TOOLS_DIR)/run_tests.sh

clean:; rm -rf .depend *.o *.cm[io] _test
watch:; fswatch -l 0.1 -o *.ml | xargs -n1 -I{} make -s test
test:$(shell $(LIST_TESTS)); $(shell $(RUN_TESTS));

_test/%tests.ml: %.ml; mkdir -p _test; qtest -o $@ extract $<
_test/%tests: _test/%tests.ml %.cmo; $(OCAML_TEST) -o $@ $(shell $(GET_LINK_ORDER) $<) $<

%.cmo: %.ml; $(OCAML) -c $<
%.cmi: %.ml $(wildcard *.mli); $(OCAML) -c $<

.depend:; ocamldep *.ml > .depend
-include .depend

.PHONY: watch clean test
