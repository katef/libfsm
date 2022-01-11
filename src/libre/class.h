/* generated */

#ifndef RE_CLASS_H
#define RE_CLASS_H

#include <stddef.h>
#include <stdint.h>

#include <fsm/fsm.h>

struct range {
	uint32_t a;
	uint32_t b;
};

struct class {
	const struct range *ranges;
	size_t count;
};

extern const struct class utf8_Adlam;
extern const struct class utf8_Ahom;
extern const struct class utf8_Anatolian_Hieroglyphs;
extern const struct class utf8_Arabic;
extern const struct class utf8_Armenian;
extern const struct class utf8_Avestan;
extern const struct class utf8_Balinese;
extern const struct class utf8_Bamum;
extern const struct class utf8_Bassa_Vah;
extern const struct class utf8_Batak;
extern const struct class utf8_Bengali;
extern const struct class utf8_Bhaiksuki;
extern const struct class utf8_Bopomofo;
extern const struct class utf8_Brahmi;
extern const struct class utf8_Braille;
extern const struct class utf8_Buginese;
extern const struct class utf8_Buhid;
extern const struct class utf8_Canadian_Aboriginal;
extern const struct class utf8_Carian;
extern const struct class utf8_Caucasian_Albanian;
extern const struct class utf8_Chakma;
extern const struct class utf8_Cham;
extern const struct class utf8_Cherokee;
extern const struct class utf8_Chorasmian;
extern const struct class utf8_Common;
extern const struct class utf8_Coptic;
extern const struct class utf8_Cuneiform;
extern const struct class utf8_Cypriot;
extern const struct class utf8_Cypro_Minoan;
extern const struct class utf8_Cyrillic;
extern const struct class utf8_Deseret;
extern const struct class utf8_Devanagari;
extern const struct class utf8_Dives_Akuru;
extern const struct class utf8_Dogra;
extern const struct class utf8_Duployan;
extern const struct class utf8_Egyptian_Hieroglyphs;
extern const struct class utf8_Elbasan;
extern const struct class utf8_Elymaic;
extern const struct class utf8_Ethiopic;
extern const struct class utf8_Georgian;
extern const struct class utf8_Glagolitic;
extern const struct class utf8_Gothic;
extern const struct class utf8_Grantha;
extern const struct class utf8_Greek;
extern const struct class utf8_Gujarati;
extern const struct class utf8_Gunjala_Gondi;
extern const struct class utf8_Gurmukhi;
extern const struct class utf8_Han;
extern const struct class utf8_Hangul;
extern const struct class utf8_Hanifi_Rohingya;
extern const struct class utf8_Hanunoo;
extern const struct class utf8_Hatran;
extern const struct class utf8_Hebrew;
extern const struct class utf8_Hiragana;
extern const struct class utf8_Imperial_Aramaic;
extern const struct class utf8_Inherited;
extern const struct class utf8_Inscriptional_Pahlavi;
extern const struct class utf8_Inscriptional_Parthian;
extern const struct class utf8_Javanese;
extern const struct class utf8_Kaithi;
extern const struct class utf8_Kannada;
extern const struct class utf8_Katakana;
extern const struct class utf8_Kayah_Li;
extern const struct class utf8_Kharoshthi;
extern const struct class utf8_Khitan_Small_Script;
extern const struct class utf8_Khmer;
extern const struct class utf8_Khojki;
extern const struct class utf8_Khudawadi;
extern const struct class utf8_Lao;
extern const struct class utf8_Latin;
extern const struct class utf8_Lepcha;
extern const struct class utf8_Limbu;
extern const struct class utf8_Linear_A;
extern const struct class utf8_Linear_B;
extern const struct class utf8_Lisu;
extern const struct class utf8_Lycian;
extern const struct class utf8_Lydian;
extern const struct class utf8_Mahajani;
extern const struct class utf8_Makasar;
extern const struct class utf8_Malayalam;
extern const struct class utf8_Mandaic;
extern const struct class utf8_Manichaean;
extern const struct class utf8_Marchen;
extern const struct class utf8_Masaram_Gondi;
extern const struct class utf8_Medefaidrin;
extern const struct class utf8_Meetei_Mayek;
extern const struct class utf8_Mende_Kikakui;
extern const struct class utf8_Meroitic_Cursive;
extern const struct class utf8_Meroitic_Hieroglyphs;
extern const struct class utf8_Miao;
extern const struct class utf8_Modi;
extern const struct class utf8_Mongolian;
extern const struct class utf8_Mro;
extern const struct class utf8_Multani;
extern const struct class utf8_Myanmar;
extern const struct class utf8_Nabataean;
extern const struct class utf8_Nandinagari;
extern const struct class utf8_New_Tai_Lue;
extern const struct class utf8_Newa;
extern const struct class utf8_Nko;
extern const struct class utf8_Nushu;
extern const struct class utf8_Nyiakeng_Puachue_Hmong;
extern const struct class utf8_Ogham;
extern const struct class utf8_Ol_Chiki;
extern const struct class utf8_Old_Hungarian;
extern const struct class utf8_Old_Italic;
extern const struct class utf8_Old_North_Arabian;
extern const struct class utf8_Old_Permic;
extern const struct class utf8_Old_Persian;
extern const struct class utf8_Old_Sogdian;
extern const struct class utf8_Old_South_Arabian;
extern const struct class utf8_Old_Turkic;
extern const struct class utf8_Old_Uyghur;
extern const struct class utf8_Oriya;
extern const struct class utf8_Osage;
extern const struct class utf8_Osmanya;
extern const struct class utf8_Pahawh_Hmong;
extern const struct class utf8_Palmyrene;
extern const struct class utf8_Pau_Cin_Hau;
extern const struct class utf8_Phags_Pa;
extern const struct class utf8_Phoenician;
extern const struct class utf8_Psalter_Pahlavi;
extern const struct class utf8_Rejang;
extern const struct class utf8_Runic;
extern const struct class utf8_Samaritan;
extern const struct class utf8_Saurashtra;
extern const struct class utf8_Sharada;
extern const struct class utf8_Shavian;
extern const struct class utf8_Siddham;
extern const struct class utf8_SignWriting;
extern const struct class utf8_Sinhala;
extern const struct class utf8_Sogdian;
extern const struct class utf8_Sora_Sompeng;
extern const struct class utf8_Soyombo;
extern const struct class utf8_Sundanese;
extern const struct class utf8_Syloti_Nagri;
extern const struct class utf8_Syriac;
extern const struct class utf8_Tagalog;
extern const struct class utf8_Tagbanwa;
extern const struct class utf8_Tai_Le;
extern const struct class utf8_Tai_Tham;
extern const struct class utf8_Tai_Viet;
extern const struct class utf8_Takri;
extern const struct class utf8_Tamil;
extern const struct class utf8_Tangsa;
extern const struct class utf8_Tangut;
extern const struct class utf8_Telugu;
extern const struct class utf8_Thaana;
extern const struct class utf8_Thai;
extern const struct class utf8_Tibetan;
extern const struct class utf8_Tifinagh;
extern const struct class utf8_Tirhuta;
extern const struct class utf8_Toto;
extern const struct class utf8_Ugaritic;
extern const struct class utf8_Vai;
extern const struct class utf8_Vithkuqi;
extern const struct class utf8_Wancho;
extern const struct class utf8_Warang_Citi;
extern const struct class utf8_Yezidi;
extern const struct class utf8_Yi;
extern const struct class utf8_Zanabazar_Square;
extern const struct class utf8_C;
extern const struct class utf8_L;
extern const struct class utf8_M;
extern const struct class utf8_N;
extern const struct class utf8_P;
extern const struct class utf8_S;
extern const struct class utf8_Z;
extern const struct class utf8_Cf;
extern const struct class utf8_Co;
extern const struct class utf8_Cs;
extern const struct class utf8_Ll;
extern const struct class utf8_Lm;
extern const struct class utf8_Lo;
extern const struct class utf8_Lt;
extern const struct class utf8_Lu;
extern const struct class utf8_Mc;
extern const struct class utf8_Me;
extern const struct class utf8_Mn;
extern const struct class utf8_Nd;
extern const struct class utf8_Nl;
extern const struct class utf8_No;
extern const struct class utf8_Pc;
extern const struct class utf8_Pd;
extern const struct class utf8_Pe;
extern const struct class utf8_Pf;
extern const struct class utf8_Pi;
extern const struct class utf8_Po;
extern const struct class utf8_Ps;
extern const struct class utf8_Sc;
extern const struct class utf8_Sk;
extern const struct class utf8_Sm;
extern const struct class utf8_So;
extern const struct class utf8_Zl;
extern const struct class utf8_Zp;
extern const struct class utf8_Zs;
extern const struct class utf8_private;
extern const struct class utf8_assigned;

extern const struct class class_alnum;
extern const struct class class_alpha;
extern const struct class class_any;
extern const struct class class_ascii;
extern const struct class class_cntrl;
extern const struct class class_digit;
extern const struct class class_graph;
extern const struct class class_hspace;
extern const struct class class_hspace_pcre;
extern const struct class class_lower;
extern const struct class class_print;
extern const struct class class_punct;
extern const struct class class_space;
extern const struct class class_spchr;
extern const struct class class_upper;
extern const struct class class_vspace;
extern const struct class class_vspace_pcre;
extern const struct class class_word;
extern const struct class class_xdigit;
extern const struct class class_nl;
extern const struct class class_bsr;
extern const struct class class_notdigit;
extern const struct class class_nothspace;
extern const struct class class_nothspace_pcre;
extern const struct class class_notspace;
extern const struct class class_notvspace;
extern const struct class class_notvspace_pcre;
extern const struct class class_notword;
extern const struct class class_notnl;

#endif
