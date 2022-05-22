#ifndef __DOT_GRAPH_HPP__
#define __DOT_GRAPH_HPP__

#include <set>

#include <llvm/Support/FormattedStream.h>

#include "../utility/debug_utility.hpp"

namespace llvm {
    namespace bphls {

template <typename T> class dotGraph {
public:
    dotGraph(formatted_raw_ostream& o,
            void (*callback_node_label)(raw_ostream &out, T *I),
            void (*callback_node_label_extras)(raw_ostream &out, T *I)=NULL
            ) :
        limit(40), out(o), printNodeLabel(callback_node_label),
        printNodeLabelExtras(callback_node_label_extras) {
        out << "digraph {\n";
    }

    ~dotGraph() {
        out << "}\n";
    }

    void printLabel(formatted_raw_ostream &out, T *I) {
        out << "[label=\"";

        std::string stripped;
        raw_string_ostream stream(stripped);

        printNodeLabel(stream, I);
        // need to flush the stream to write to string!
        stream.flush();

        // newlines aren't allowed in dot labels
        replaceAll(stripped, "\n", " ");

        // limit the size of the instruction string
        limitString(stripped, limit);

        out << stripped;

        out << "\"";

        if (printNodeLabelExtras) {
            printNodeLabelExtras(out, I);
        }

        out << "]";
    }

    void printNode(formatted_raw_ostream &out, T *I) {
        out << "Node" << static_cast<const void*>(I);
    }

    void connectDot(formatted_raw_ostream &out, T *driver,
            T *signal, std::string label) {

        if (seen.find(signal) == seen.end()) {
            printNode(out, signal);
            printLabel(out, signal);
            out << ";\n";
            seen.insert(signal);
        }

        if (seen.find(driver) == seen.end()) {
            printNode(out, driver);
            printLabel(out, driver);
            out << ";\n";
            seen.insert(driver);
        }

        printNode(out, driver);
        out << " -> ";
        printNode(out, signal);
        if (!label.empty()) out << "[" << label << "]";
        out << ";\n";
    }

    void setLabelLimit(int l) { limit = l; }

private:
    std::set<T*> seen;
    unsigned limit;
    llvm::formatted_raw_ostream &out;
    void (*printNodeLabel)(raw_ostream &out, T *I);
    void (*printNodeLabelExtras)(raw_ostream &out, T *I);
};

    } /* namespace bphls */
} /* namespace llvm */

#endif /* __DOT_GRAPH_HPP__ */
