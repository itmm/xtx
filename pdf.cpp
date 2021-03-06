#include "pdf.h"

#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "cmd.h"
#include "err.h"
#include "eval.h"
#include "invocation.h"
#include "map.h"
#include "print.h"
#include "token.h"

Map_Writer::Map_Writer(Pdf_Writer *root): Sub_Writer { root, "    " } {
	*root << "  <<\n";
}

Map_Writer::~Map_Writer() {
	*root() << "  >>\n";
}

List_Writer::List_Writer(Pdf_Writer *root): Sub_Writer { root, " " } {
	*root << '[';
}

List_Writer::~List_Writer() {
	*this << "]\n";
}

Obj_Writer::Obj_Writer(int id, Pdf_Writer *root): Sub_Writer { root, "    " } {
	root->define_obj(id);
	*root << id << " 0 obj\n";
}

Obj_Writer::~Obj_Writer() {
	auto stream_length = root()->position() - stream_start_;
	if (stream_start_ > 0) {
		*root() << "endstream\n";
		stream_start_ = -1;
	}
	*root() << "endobj\n";
	if (stream_length_id_ > 0) {
		int id { stream_length_id_ };
		stream_length_id_ = -1;
		{
			Obj_Writer obj { id, root() };
			obj << stream_length << '\n';
		}
	}
}

void Obj_Writer::open_stream(std::unique_ptr<Map_Writer> map) {
	assert(stream_length_id_ < 0);
	assert(stream_start_ < 0);
	stream_length_id_ = root()->reserve_obj_id();
	*map << "/Length " << stream_length_id_ << " 0 R\n";
	map.reset();
	*root() << "stream\n";
	stream_start_ = root()->position();
}

void Pdf_Writer::write_header() {
	out_ << "%PDF-1.4\n";
}

Pdf_Writer::Pdf_Writer(std::ostream &out): out_ { out } {
	write_header();
	root_id_ = reserve_obj_id();
	int pages = reserve_obj_id();
	{
		Obj_Writer obj { root_id_, this };
		{
			Map_Writer map { this };
			map << "/Type /Catalog\n";
			map << "/Pages " << pages << " 0 R\n";
		}
	}
	int page = reserve_obj_id();
	{
		Obj_Writer obj { pages, this };
		{
			Map_Writer map { this };
			map << "/Type /Pages\n";
			map << "/Count 1\n";
			{
				map << "/Kids ";
				List_Writer lst { this };
				lst << page << " 0 R";
			}
		}
	}
	int font_id = reserve_obj_id();
	int content_id = reserve_obj_id();
	{
		Obj_Writer obj { page, this };
		{
			Map_Writer map { this };
			map << "/Type /Page\n";
			map << "/Parent " << pages << " 0 R\n";
			{
				map << "/MediaBox ";
				List_Writer lst { this };
				lst << 0;
				lst << 0;
				lst << 612;
				lst << 792;
			}
			{
				map << "/Resources ";
				Map_Writer resources { this };
				{
					resources << "/Font ";
					Map_Writer font { this };
					font << "/F0 " << font_id << " 0 R\n";
				}
			}
			map << "/Contents " << content_id << " 0 R\n";
		}
	}
	{
		Obj_Writer obj { font_id, this };
		{
			Map_Writer map { this };
			map << "/Type /Font\n";
			map << "/Subtype /Type1\n";
			map << "/BaseFont /TimesRoman\n";
		}
	}

	{
		content_ = std::make_unique<Obj_Writer>(content_id, this);
		auto map = std::make_unique<Map_Writer>(this);
		content_->open_stream(std::move(map));
	}
	out_ << "BT\n";
	out_ << "    /F0 12 Tf\n";
	out_ << "    72 720 Td\n";
	out_ << "    14 TL\n";
}

Pdf_Writer::~Pdf_Writer() {
	out_ << "ET\n";
	content_.reset();
	write_trailer();
}

void Pdf_Writer::write_xref() {
	out_ << "xref\n";
	out_ << "0 " << (last_obj_id_ + 1) << '\n';
	for (int i = 0; i <= last_obj_id_; ++i) {
		auto pos { obj_positions_[i] };
		if (pos) {
			out_ << std::setfill('0') << std::setw(10) << pos << ' ';
			out_ << "00000 n \n";
		} else {
			out_ << "0000000000 65535 f \n";
			if (pos > 0) {
				std::cerr << "no object for id " << i << "\n";
			}
		}
	}
}

void Pdf_Writer::write_trailer_dict() {
	Map_Writer trailer { this };
	trailer << "/Size " << (last_obj_id_ + 1) << '\n';
	trailer << "/Root " << root_id_ << " 0 R\n";
}

void Pdf_Writer::write_trailer() {
	auto xref { position() };
	write_xref();
	out_ << "trailer\n";
	write_trailer_dict();
	out_ << "startxref\n";
	out_ << xref << '\n';
	out_ << "%%EOF\n";
}

void Pdf_Writer::write_log(const std::string &line) {
	out_ << "    (";
	for (char c : line) {
		switch (c) {
		case '(': case ')': case '\\':
			out_ << '\\'; // fallthrough
		default:
			out_ << c;
		}
	}
	out_ << ") '\n";
}

class Pdf_Cmd: public Command {
public:
	[[nodiscard]] Node_Ptr eval(Node_Ptr invocation, Node_Ptr state) const override;
};

static Invocation::Iter eat_space(Invocation::Iter it, Invocation::Iter end) {
	while (it != end && (**it).as_space()) { ++it; }
	return it;
}

Node_Ptr Pdf_Cmd::eval(Node_Ptr invocation, Node_Ptr state) const {
	auto it { invocation->as_invocation()->begin() };
	auto end { invocation->as_invocation()->end() };
	it = eat_space(it, end);
	if (it != end) { ++it; }
	it = eat_space(it, end);
	if (it == end || ! (**it).as_token() || (**it).as_token()->token() != "as:") {
		err("pdf: expected as: parameter");
	}
	it = eat_space(++it, end);
	if (it == end) { err("no as: value"); }
	Node_Ptr path { ::eval(*it++, state) };
	if (! path || ! path->as_token()) {
		err("no as: token value");
	}

	std::ofstream out(path->as_token()->token().c_str());
	Pdf_Writer writer(out);

	it = eat_space(it, end);
	if (it == end || ! (**it).as_token() || (**it).as_token()->token() != "with:") {
		err("pdf: expected with: parameter");
	}
	it = eat_space(++it, end);

	Node_Ptr value;

	while (it != end) {
		std::ostringstream line;
		for (; it != end && *it && ! (**it).as_space(); ++it) {
			value = *it;
			if (value->as_invocation()) {
				value = ::eval(value, state);
				if (! value || value->as_space()) {
					if (! line.str().empty()) {
						writer.write_log(line.str());
					}
					++it; break;
				}
			}
			line << value;
		}
		if (! line.str().empty()) {
			writer.write_log(line.str());
		}
		it = eat_space(it, end);
	}
	return value;
}

void add_pdf_commands(const Node_Ptr &state) {
	Map *s { state->as_map() };
	s->push(std::make_shared<Pdf_Cmd>(), "pdf");
}

void Pdf_Writer::define_obj(int id) {
	assert(id <= last_obj_id_);
	assert(! obj_positions_[id]);
	obj_positions_[id] = position();
}
