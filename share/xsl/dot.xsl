<?xml version="1.0"?>

<xsl:stylesheet version="1.0" 
	xmlns="http://www.w3.org/2000/svg"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:gv="http://xml.libfsm.org/gv">

	<xsl:template match="/">
		<xsl:processing-instruction name="xml-stylesheet">href="http://www.libfsm.org/css/dot.css" type="text/css"</xsl:processing-instruction>
		<xsl:comment>kate` on freenode. kate@elide.org</xsl:comment>

		<xsl:apply-templates/>
	</xsl:template>

	<xsl:template match="svg:svg">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>

			<xsl:apply-templates select="node()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:*">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:a">
		<xsl:apply-templates select="node()"/>
	</xsl:template>

	<xsl:template match="@*">
		<xsl:copy/>
	</xsl:template>

	<xsl:template match="svg:g">
		<xsl:variable name="from" select="substring-before(svg:title, '-')"/>
		<xsl:variable name="to"   select="substring-after(svg:title, '&gt;')"/>

		<xsl:copy>
			<xsl:attribute name="gv:edge">
				<xsl:value-of select="concat($from, '-', $to)"/>
			</xsl:attribute>

			<!-- in our .dot, tooltip="1 2 3" specifies the reachable set -->
			<xsl:if test="svg:a/@xlink:title">
				<xsl:attribute name="gv:reachable">
					<xsl:value-of select="svg:a/@xlink:title"/>
				</xsl:attribute>
			</xsl:if>

			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:g[@class = 'node']">
		<xsl:variable name="id" select="svg:title"/>

		<a gv:node="{$id}">
			<!-- in our .dot, URL="http://..." specifies the link URL, if present -->
			<xsl:if test="svg:a/@xlink:href">
				<xsl:attribute name="xlink:href">
					<xsl:value-of select="svg:a/@xlink:href"/>
				</xsl:attribute>
			</xsl:if>

			<!-- in our .dot, target="/a.*bc?" specifies regexps to match URLs -->
			<xsl:if test="svg:a/@target">
				<xsl:attribute name="gv:match">
					<xsl:value-of select="svg:a/@target"/>
				</xsl:attribute>
			</xsl:if>

			<xsl:if test="count(svg:ellipse) = 2">
				<xsl:attribute name="gv:end">
					<xsl:text>end</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<!-- this node -->
			<xsl:apply-templates select=".//svg:ellipse"/>

			<!-- edges from this node -->
			<xsl:apply-templates select="../svg:g[@class = 'edge']
				[substring-before(svg:title, '-') = $id]"/>
		</a>
	</xsl:template>

	<xsl:template match="svg:svg/svg:g">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>

			<xsl:apply-templates select="svg:g[@class = 'node']"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:text">
		<xsl:variable name="text" select="translate(., ' _', '')"/>

		<xsl:if test="$text != ''">
			<xsl:copy>
				<xsl:apply-templates select="@*|node()|text()"/>
			</xsl:copy>
		</xsl:if>
	</xsl:template>

	<xsl:template match="svg:g[@class = 'node' and svg:title = 'start']"/>
	<xsl:template match="/svg:svg/svg:g/svg:polygon[@fill = 'white']"/>

	<xsl:template match="/svg:svg/@width"/>
	<xsl:template match="/svg:svg/@height"/>
	<xsl:template match="/svg:svg/@viewBox"/>
	<xsl:template match="@fill[. != 'none']"/>
	<xsl:template match="svg:ellipse/@fill"/>
	<xsl:template match="@font-family"/>
	<xsl:template match="@stroke"/>
	<xsl:template match="@id"/>
	<xsl:template match="@class"/>
	<xsl:template match="svg:title"/>
	<xsl:template match="svg:path   [@fill = 'none' and @stroke = 'none']"/>
	<xsl:template match="svg:polygon[@fill = 'none' and @stroke = 'none']"/>

</xsl:stylesheet>

