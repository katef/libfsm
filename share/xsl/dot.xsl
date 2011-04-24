<?xml version="1.0"?>

<xsl:stylesheet  version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/2000/svg"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:xi="http://www.w3.org/2001/XInclude">

	<!-- TODO: use base.xsl for css location etc -->

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

	<xsl:template match="@*">
		<xsl:copy/>
	</xsl:template>

	<xsl:template match="svg:g">
		<xsl:copy>
			<xsl:if test="@class = 'edge'">
				<xsl:variable name="id" select="substring-before(svg:title, '-')"/>

				<xsl:attribute name="gv-id">
					<xsl:text>node</xsl:text>
					<xsl:value-of select="$id"/>
				</xsl:attribute>

				<xsl:attribute name="onmouseover">
					<xsl:text>msOver('</xsl:text>
					<xsl:value-of select="$id"/>
					<xsl:text>')</xsl:text>
				</xsl:attribute>

				<xsl:attribute name="onmouseout">
					<xsl:text>msOut('</xsl:text>
					<xsl:value-of select="$id"/>
					<xsl:text>')</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<xsl:if test="@class = 'node'">
				<xsl:variable name="id" select="svg:title"/>

				<xsl:attribute name="gv-id">
					<xsl:text>node</xsl:text>
					<xsl:value-of select="$id"/>
				</xsl:attribute>

				<xsl:attribute name="onmouseover">
					<xsl:text>msOver('</xsl:text>
					<xsl:value-of select="$id"/>
					<xsl:text>')</xsl:text>
				</xsl:attribute>

				<xsl:attribute name="onmouseout">
					<xsl:text>msOut('</xsl:text>
					<xsl:value-of select="$id"/>
					<xsl:text>')</xsl:text>
				</xsl:attribute>
			</xsl:if>

			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="svg:g[@class = 'node' and svg:title = 'start']"/>
	<xsl:template match="/svg:svg/svg:g/svg:polygon[@fill = 'white']"/>

<!--
	<xsl:template match="/svg:svg/@width"/>
	<xsl:template match="/svg:svg/@height"/>
	<xsl:template match="/svg:svg/@viewBox"/>
-->
	<xsl:template match="@fill[. != 'none']"/>
	<xsl:template match="@font-family"/>
	<xsl:template match="@stroke"/>
	<xsl:template match="@id"/>
	<xsl:template match="@class"/>
	<xsl:template match="svg:title"/>

</xsl:stylesheet>

