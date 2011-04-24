<?xml version="1.0" standalone="yes"?>

<!-- $Id$ -->

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/1999/xhtml"

	xmlns:common="http://exslt.org/common"
	xmlns:str="http://exslt.org/strings"

	extension-element-prefixes="common str">

	<xsl:template name="output">
		<xsl:param name="filename"/>
		<xsl:param name="title"/>
		<xsl:param name="css" select="''"/>
		<xsl:param name="js"  select="''"/>

		<xsl:param name="content.head" select="/.."/>
		<xsl:param name="content.body" select="/.."/>

		<xsl:variable name="method">
			<xsl:choose>
				<xsl:when test="$libfsm.ext = 'xhtml'">
					<xsl:value-of select="'xml'"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="'html'"/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
				

		<!-- TODO: have these as a database of tags in this .xsl file and use document() to get them -->
		<xsl:variable name="media">
			<xsl:choose>
				<xsl:when test="$method = 'xml'">
					<xsl:text>application/xhtml+xml'/xml</xsl:text>
				</xsl:when>
				<xsl:when test="$method = 'html'">
					<xsl:text>text/html</xsl:text>
				</xsl:when>
			</xsl:choose>
		</xsl:variable>

		<xsl:variable name="doctype-public">
			<xsl:choose>
				<xsl:when test="$method = 'xml'">
					<xsl:text>-//W3C//DTD XHTML 1.0 Strict//EN</xsl:text>
				</xsl:when>
				<xsl:when test="$method = 'html'">
					<xsl:text>TODO</xsl:text>
				</xsl:when>
			</xsl:choose>
		</xsl:variable>

		<xsl:variable name="doctype-system">
			<xsl:choose>
				<xsl:when test="$method = 'xml'">
					<xsl:text>DTD/xhtml1-strict.dtd</xsl:text>
				</xsl:when>
				<xsl:when test="$method = 'html'">
					<xsl:text>TODO</xsl:text>
				</xsl:when>
			</xsl:choose>
		</xsl:variable>

		<!-- TODO: default to stdout, if no filename is given? -->
		<xsl:variable name="output-file" select="concat($filename, '.', $libfsm.ext)"/>

		<xsl:message>
			<xsl:text>Outputting </xsl:text>
			<xsl:value-of select="concat($output-file, ': &quot;', $title, '&quot;')"/>
		</xsl:message>

		<common:document
			href="{$output-file}"
			method="{$method}"
			encoding="utf-8"
			indent="yes"
			omit-xml-declaration="no"
			cdata-section-elements="script"
			media-type="{$media}"
			doctype-public="{$doctype-public}"
			doctype-system="{$doctype-system}"
			standalone="yes">

			<xsl:call-template name="output-content">
				<xsl:with-param name="title"   select="$title"/>
				<xsl:with-param name="method"  select="$method"/>
				<xsl:with-param name="css"     select="$css"/>
				<xsl:with-param name="js"      select="$js"/>
				<xsl:with-param name="content.head" select="$content.head"/>
				<xsl:with-param name="content.body" select="$content.body"/>
			</xsl:call-template>

		</common:document>
	</xsl:template>

	<xsl:template name="output-content">
		<xsl:param name="title"/>
		<xsl:param name="method"       select="'xml'"/>
		<xsl:param name="css"          select="''"/>
		<xsl:param name="js"           select="''"/>
		<xsl:param name="content.head" select="/.."/>
		<xsl:param name="content.body" select="/.."/>

		<html>
			<head>
				<title>
					<xsl:value-of select="$title"/>
					<xsl:text> &#8211; libfsm</xsl:text>
				</title>

				<!-- TODO: maybe a node set is better, after all -->
				<xsl:for-each select="str:tokenize(concat(
					' ', $libfsm.url.www, '/css/grid.css',
					' ', $libfsm.url.www, '/css/typography.css',
					' ', 'http://fonts.googleapis.com/css?family=Quattrocento',
					' ', $css))">
					<link rel="stylesheet" type="text/css" media="screen" href="{.}"/>
				</xsl:for-each>

<!-- TODO: linenumbers only for docbook and wiki pages? -->
<!-- TODO:
				<xsl:for-each select="str:tokenize(concat(
					' ', $libfsm.url.www, '/js/debug.js',
					' ', $libfsm.url.www, '/js/linenumbers.js',
					' ', $libfsm.url.www, '/js/table.js',
					' ', $libfsm.url.www, '/js/col.js',
					' ', $libfsm.url.www, '/js/javascript-xpath-cmp.js',
					' ', $js))">
					<script type="text/javascript" src="{.}"></script>
				</xsl:for-each>
-->

				<!-- TODO: meta headers for prev/next links -->

				<xsl:if test="$method = 'html'">
					<meta http-equiv="Content-Type"
						content="text/html; charset=utf-8"/>
				</xsl:if>

				<xsl:copy-of select="$content.head"/>
			</head>

			<!-- TODO: only load javascript if $content contains appropiate things (or pass in explicitly stating what to load) -->
			<!-- TODO: not documentElement, but the 'main' div within the body, if there is one -->
			<body>
<!-- TODO:
 onload="var r = document.documentElement;
				Linenumbers.init(r);
				Colalign.init(r);
				Table.init(r)">
-->

				<xsl:copy-of select="$content.body"/>
			</body>
		</html>
	</xsl:template>

</xsl:stylesheet>

